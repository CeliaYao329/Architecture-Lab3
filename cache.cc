#include "cache.h"
#include "def.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

inline int lg2(const int x)
{
    int num = 0;
    int xx = x;
    while(xx != 1)
    {
        xx = xx >> 1;
        num ++;
    }
    return num;
}

Cache::Cache(){
  config_.size = 32; 
  config_.associativity = 8;
  config_.set_num = 64; //set_num = size * 1024 / associativity / block_size 
  config_.block_size = 64; //byte
  config_.write_through = 0;
  config_.write_allocate = 1;
  lower_ = NULL;
  cache_ = new Block *[config_.set_num];
  for (int i = 0; i < config_.set_num ; i++){
    cache_[i] = new Block[config_.associativity];
    memset(cache_[i], 0 ,sizeof(Block)*config_.associativity);
    for (int j = 0; j < config_.associativity; j++){
      cache_[i][j].block_content = new char[config_.block_size];
    }
  } 
}


void Cache::SetConfig(CacheConfig cc){
  for(int i = 0; i< config_.set_num ; i++){
    for(int j = 0; j < config_.associativity;j++){
      delete [] cache_[i][j].block_content;
    }
    delete []cache_[i];
  }
  delete []cache_;

  config_.size = cc.size;
  config_.associativity = cc.associativity;
  config_.set_num = cc.set_num; //set_num = size * 1024 / associativity / block_size 
  config_.block_size = cc.block_size; //byte
  config_.write_through = cc.write_through;
  config_.write_allocate = cc.write_allocate;

  printf("\n\n------------Cache Setting------------\n\nsize = %d, block_size = %d, assc = %d, set_num = %d\n",
           config_ .size, config_ .block_size, config_ .associativity, config_ .set_num);
  printf("write_through(back) = %d, write_alloc(no-alloc) = %d\n", config_ .write_through, config_ .write_allocate);

  cache_ = new Block *[config_.set_num];
  for (int i = 0; i < config_.set_num ; i++){
    cache_[i] = new Block[config_.associativity];
    memset(cache_[i], 0 ,sizeof(Block)*config_.associativity);
    for (int j = 0; j < config_.associativity; j++){
      cache_[i][j].block_content = new char[config_.block_size];
    }
  }
}

void Cache::GetConfig(CacheConfig cc){
    cc.size = config_.size;
    cc.block_size = config_.block_size;
    cc.associativity = config_.associativity;
    cc.set_num = config_.set_num;
    cc.write_through = config_.write_through;
    cc.write_allocate = config_.write_allocate;
}

void Cache::HandleRequest(unsigned int addr, int bytes, int read,
                          char *content, int &hit, int &time) {

  int b_bit = lg2(config_.block_size);
  int s_bit = lg2(config_.set_num); 
  unsigned int set_index = (unsigned int)GETSET(addr, b_bit+s_bit,b_bit);
  unsigned int tag = (unsigned int)GETTAG(addr,s_bit+b_bit);
  unsigned int block_offset = (unsigned int)GETOFFSET(addr,b_bit);
  
  hit = 0;

  time += latency_.bus_latency;
  stats_.access_time += latency_.bus_latency;//????

  //printf("\naddr = 0x%llx(%lld), b_bit = %d, s_bit = %d\n", addr, addr, b_bit, s_bit);
  //printf("addr=0x%llx(%lld), set_index = 0x%x, tag = 0x%x, offset = 0x%x, read = %d\n", addr, addr, set_index, tag, block_offset, read);

  //there are contents from other blocks --> handle seperately
  int overbytes = block_offset + bytes - config_.block_size;
  /*if(overbytes > 0) {
    printf("\naddr = 0x%llx(%lld), b_bit = %d, s_bit = %d\n", addr, addr, b_bit, s_bit);
    //printf("set_index = 0x%x, tag = 0x%x, offset = 0x%x, read = %d\n", set_index, tag, block_offset, read);

    printf("Multiple blocks: block_offset: %d, bytes: %d, block_size: %d\n",block_offset , bytes, config_.block_size);
    int bytes_firstblock = config_.block_size - block_offset + 1; //????
    HandleRequest(addr, bytes_firstblock, read, content, hit, time);
    HandleRequest(addr + bytes_firstblock, overbytes, read, content + bytes_firstblock, hit, time);
    //如果有跨越两个block的情况 一个hit就算hit???
    return;
  }*/

  // Bypass?
  if (!BypassDecision()) {
    PartitionAlgorithm();
    
    time += latency_.hit_latency;
    stats_.access_time += latency_.hit_latency;

    unsigned int target_block;
    stats_.access_counter++;
    // IF HIT
    int condition = 0;
    condition = isHit(set_index, tag, target_block);
    //printf("condition: %d\n",condition);
    
    if(condition == HIT){
      hit = 1;
      SetRPP(set_index, cache_[set_index][target_block]);
      
      //READ HITm
      if(read){
        printf("HIT read\n");
        memcpy(content, cache_[set_index][target_block].block_content + block_offset, bytes);
        SetRPP(set_index,cache_[set_index][target_block]);
        return;
      }
      //WRITE HIT
      else{
        printf("HIT write\n");
        memcpy(cache_[set_index][target_block].block_content + block_offset, content, bytes);
        
        if(!config_.write_through){
          //WRITE HIT not_write_through: defer write to memory until replacement of line, need a dirty bit(line different from memory or not)
          cache_[set_index][target_block].dirty_bit = 1;
        }
        else{
          int lower_hit = 0, lower_time = 0;
          lower_ -> HandleRequest(addr, bytes, read, content, lower_hit, time);
          stats_.fetch_num++;
        }
      }
    }

    //IF MISS
    else{
      hit = 0;
      stats_.miss_num++;

      //WRITE MISS write_no_allocate: writes straight to memory, does not load into cache
      if(read == 0 && !config_.write_allocate){
      	int lower_hit = 0;
        lower_ -> HandleRequest(addr, bytes, read, content, hit, time);
        stats_.fetch_num++;
        return;
      }

      //evict the block
      if(target_block == -1){
        printf("MISS need to evict a block");
        target_block = ReplaceAlgorithm(set_index);
        //printf("MISS Evict block: 0x%x\n",target_block);
        stats_.replace_num ++;
        if(config_.write_through == 0 && cache_[set_index][target_block].dirty_bit == 1){
          int lower_hit = 0;
          uint64_t evict_address = (tag<<(b_bit+s_bit)) || (set_index << b_bit);
          //uint64_t evict_address = addr - block_offset;
          char *evict_content = new char[config_.block_size];
          memcpy(evict_content,cache_[set_index][target_block].block_content,config_.block_size);
          lower_->HandleRequest(evict_address, config_.block_size, 0 , evict_content, lower_hit, time);
          stats_.fetch_num++; 
          cache_[set_index][target_block].dirty_bit = 0;
        }
        cache_[set_index][target_block].valid_bit = 0;
      }
      //else printf("MISS available block: 0x%x\n", target_block);


      if(read){
        printf("miss read\n");
        //READ MISS with available slot
        char lower_content[config_.block_size];
        int lower_hit = 0;
        lower_-> HandleRequest(addr - block_offset, config_.block_size, read, lower_content, lower_hit, time);
        stats_.fetch_num++;
        memcpy(cache_[set_index][target_block].block_content,lower_content,config_.block_size);
        cache_[set_index][target_block].tag = tag;
        cache_[set_index][target_block].valid_bit = 1;
        cache_[set_index][target_block].dirty_bit = 0;
        //printf("Load cache_[0x%x][0x%x]: set valid_bit %d\n", set_index, target_block, cache_[set_index][target_block].valid_bit);
        SetRPP(set_index,cache_[set_index][target_block]);
        memcpy(content, cache_[set_index][target_block].block_content + block_offset, bytes);
      }

      else{
        printf("miss write\n");
        if(config_.write_allocate){
          //WRITE MISS write_allocate: load into cache, update line in cache
          if(!config_.write_through){ 
            //WRITE MISS write_allocate no_write_through
            //load to cache and set dirty_bit
            memcpy(cache_[set_index][target_block].block_content + block_offset, content, bytes);
            cache_[set_index][target_block].valid_bit = 1;
            cache_[set_index][target_block].dirty_bit = 1;
            cache_[set_index][target_block].tag = tag;
            SetRPP(set_index, cache_[set_index][target_block]);
          }
          else{
            //WRITE MISS write_allocate write_through
            memcpy(cache_[set_index][target_block].block_content + block_offset, content, bytes);
            cache_[set_index][target_block].valid_bit = 1;
            cache_[set_index][target_block].tag = tag;
            SetRPP(set_index, cache_[set_index][target_block]);
            int lower_hit = 0, lower_time;
            lower_ -> HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
            stats_.fetch_num ++;
          } 
        } 
      }
    }

  }
  //stats_.access_time += time;
  /*if (ReplaceDecision()) {
    // Choose victim
    ReplaceAlgorithm();
    } 
    else {
      // return hit & time
      hit = 1;
      time += latency_.bus_latency + latency_.hit_latency;
      stats_.access_time += time;
      return;
    }
  
  // Prefetch?
 if (PrefetchDecision()) {
    PrefetchAlgorithm();
  } 
  else {
    // Fetch from lower layer
    int lower_hit, lower_time;
    lower_->HandleRequest(addr, bytes, read, content,
                          lower_hit, lower_time);
    hit = 0;
    time += latency_.bus_latency + lower_time;
    stats_.access_time += latency_.bus_latency;
  }*/
}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

int Cache::ReplaceDecision() { //=1 miss, should choose victim


  return TRUE;
  //return FALSE;
}

unsigned int Cache::ReplaceAlgorithm(const int set_id){
  int ev_block = -1;
  uint64_t min = UINT64_MAX;
  for (int i = 0; i < config_.associativity; i++){
    if(cache_[set_id][i].RPP_tag < min){
      min = cache_[set_id][i].RPP_tag;
      ev_block = i;
    }
  }
  return ev_block;
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

void Cache::SetRPP(const int set_id, Block & block){
  block.RPP_tag = clock();
}

int Cache::isHit(unsigned int set_index, unsigned int tag, unsigned int& target_block){
    //printf("Cache isHit? -----------------\nset_index: 0x%x, tag: 0x%x\n", set_index, tag);
    target_block = -1;
    for (int i = 0; i < config_.associativity; i++){
      printf("cache_[0x%x][0x%x].tag: 0x%x\n", set_index, i, cache_[set_index][i].tag);
      if(cache_[set_index][i].valid_bit && cache_[set_index][i].tag == tag){
        target_block = i;
        //printf("Hit to the set: 0x%x, block: 0x%x\n", set_index, target_block);
        return HIT;
      }
      if(target_block == -1 && cache_[set_index][i].valid_bit == 0){
        target_block = i;
      }
    }
    return MISS;
}

