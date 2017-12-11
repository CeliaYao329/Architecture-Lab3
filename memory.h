#ifndef CACHE_MEMORY_H_
#define CACHE_MEMORY_H_

#include <stdint.h>
#include "storage.h"

#define MAX 0x1000000

class MemoryStorage: public Storage {
 public:
 	unsigned char mem[MAX];
 	MemoryStorage();
  	~MemoryStorage(){};

  // Main access process
  void HandleRequest(unsigned int addr, int bytes, int read,
                     char *content, int &hit, int &time);

 private:
  	// Memory implement
  	DISALLOW_COPY_AND_ASSIGN(MemoryStorage);
};

#endif //CACHE_MEMORY_H_ 
