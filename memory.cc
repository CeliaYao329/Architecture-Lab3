#include "def.h"
#include "memory.h"

void MemoryStorage::HandleRequest(unsigned int addr, int bytes, int read,
                          char *content, int &hit, int &time) {
  hit = 1;
  time = latency_.hit_latency + latency_.bus_latency;
  stats_.access_time += time;
  stats_.access_counter++;

  /*if(read){
  	memcpy(content, mem + addr, bytes);
  }
  else{
  	memcpy(mem + addr, content, bytes);
  }*/
}

MemoryStorage::MemoryStorage(){
}