#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include <math.h>
#include "storage.h"
#include "def.h"

#define HIT 1
#define MISS 0

#define ONES(x,y)       (uint64_t) ((((uint64_t)1<<x)-1)+((uint64_t)1<<x) -(((uint64_t)1<<y)-1))

#define GETSET(addr, offset_tag, offset_set)    ((addr & ONES((offset_tag-1),offset_set)) >> offset_set)
#define GETTAG(addr, offset_tag)                ((addr & ONES(63,offset_tag)) >> offset_tag)
#define GETOFFSET(addr, offset_set)             (addr & ONES((offset_set-1), 0))

typedef struct CacheConfig_ {
  int size;
  int associativity; //Number of lines in each set
  int set_num; // Number of cache sets
  int block_size;
  int write_through; // 0|1 for back|through
  int write_allocate; // 0|1 for no-alc|alc
  //What to do on a write-hit?
  //write back: defer write to memory until replacement of line, need a dirty bit(line different from memory or not)
  //write through: write immediately to memory
  //What to do on a write-miss?
  //write_allocate: load into cache, update line in cache
  //no_write_allocate: writes straight to memory, does not load into cache
  //Write-back + Write-allocate
} CacheConfig;

typedef struct Block_{
  bool valid_bit;
  bool dirty_bit;
  unsigned int tag;
  uint64_t RPP_tag;
  char * block_content;
} Block;


class Cache: public Storage {
 public:
  Cache(); // done
  ~Cache() {}

  // Sets & Gets
  void SetConfig(CacheConfig cc); //done
  void GetConfig(CacheConfig cc); //done
  void SetLower(Storage *ll) { lower_ = ll; } //set lower level storage
  // Main access process
  void HandleRequest(unsigned int addr, int bytes, int read,
                     char *content, int &hit, int &time);

 private:
  // Bypassing
  int BypassDecision();
  // Partitioning
  void PartitionAlgorithm();
  // Replacement
  int isHit(unsigned int set_index,unsigned int tag, unsigned int& target_block);
  int ReplaceDecision();
  unsigned int ReplaceAlgorithm(const int set);
  void SetRPP(const int set_id, Block & block);
  // Prefetching
  int PrefetchDecision();
  void PrefetchAlgorithm();

  CacheConfig config_;
  Storage *lower_;
  Block **cache_;
  DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
