#include "stdio.h"
#include "advancedCache.h"
#include "advancedMemory.h"
#include <fstream>  
#include <sstream>  
#include <string> 
#include <memory.h>
using namespace std;

char *filename = NULL;
FILE *trace = NULL;

/*trace = fopen(filename, "r");
    if (trace != NULL) {
        return true;
    }
    return false;
}*/

CacheConfig config_cache();
void Simulation(unsigned int & total_cnt, unsigned int & total_hit, unsigned int & success_cnt, Cache & l1);
CacheConfig config_cache(int size, int associativity, int block_size, int write_through, int write_allocate, int ev_tags_num);
MemoryStorage m;
Cache l1;
Cache l2;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim <trace_file>\n");
        return 1;
    }
    filename = argv[1];
    if ((trace = fopen(filename,"r")) == NULL ) {
        printf("Error: Failed to open trace file %s.\n",argv[1]);
        return 1;
    }
    fclose(trace);
    
    // Initialize storage system
    l1.SetLower(&l2);
    l2.SetLower(&m);

    CacheConfig l1_config = config_cache(32 * 1024, 8, 64, 0, 1, 3);
    l1.SetConfig(l1_config);
    CacheConfig l2_config = config_cache(256 * 1024, 8, 64, 0, 1, 3);
    l2.SetConfig(l2_config);
    
    StorageStats s;
    memset(&s, 0, sizeof(s));
    m.SetStats(s);
    l1.SetStats(s);
    l2.SetStats(s);

    
    StorageLatency memory_latency;
    memory_latency.bus_latency = 10;
    memory_latency.hit_latency = 100;
    m.SetLatency(memory_latency);
    
    StorageLatency l1_latency;
    l1_latency.bus_latency = 0;
    l1_latency.hit_latency = 10;
    l1.SetLatency(l1_latency);

    StorageLatency l2_latency;
    l2_latency.bus_latency = 6;
    l2_latency.hit_latency = 10;
    l2.SetLatency(l2_latency);
    
    unsigned int l1_total_cnt = 0;
    unsigned int l1_total_hit = 0;
    unsigned int l1_success_cnt = 0;
    
    unsigned int l2_total_cnt = 0;
    unsigned int l2_total_hit = 0;
    unsigned int l2_success_cnt = 0;
    
    /*ofstream outFile;
    outFile.open("res.csv", ios::out);
    outFile << "Cache size" << ',' << "Block_size" << ',' << "associativity" << ',' << "Set_num" << ',' 
        << "Write Policy" << ',' << "Allocate Policy" << ',' << "Total Hit" << ',' << "Hit Rate" << ',' << "L1 Access Time" << ',' << "Mem Access Time" << endl;*/

    Simulation(l1_total_cnt, l1_total_hit, l1_success_cnt, l1);

    StorageStats l1_stats, l2_stats, m_stats;
    l1.GetStats(l1_stats);
    l2.GetStats(l2_stats);
    m.GetStats(m_stats);

    printf("cache1: access counter: %d, total miss: %d, miss rate: %.3f, access_time: %d\n", l1_stats.access_counter, l1_stats.miss_num, double(l1_stats.miss_num)/double(l1_stats.access_counter), l1_stats.access_time);
    printf("cache2: access counter: %d, total miss: %d, miss rate: %.3f, access_time: %d\n", l2_stats.access_counter, l2_stats.miss_num, double(l2_stats.miss_num)/double(l2_stats.access_counter),l2_stats.access_time);
    printf("memory: access counter: %d, access_time: %d\n", m_stats.access_counter, m_stats.access_time);
    printf("total latency: %d\n", l1_stats.access_time + l2_stats.access_time + m_stats.access_time);
    
    //printf("\n");
    //printf("Cache Simulation finished.\n");
    //printf("%u requests handled in total\n", total_cnt);
    //printf("%u requests handled successfully.\n",success_cnt);
    
    //printf("\n");
    //printf("Total hits: %d/%d\n", total_hit, success_cnt);
    //l1.GetStats(s);
    //printf("L1 access time: %dns\n", s.access_time);
    //m.GetStats(s);
    //printf("Memory access time: %dns\n", s.access_time);
    return 0;
}

void Simulation(unsigned int & total_cnt, unsigned int & total_hit, unsigned int & success_cnt, Cache & l1){
    printf("start Simulation: ");
    if ((trace = fopen(filename,"r")) == NULL ) {
        printf("Error: Failed to open trace file %s.\n",filename);
        return;
    }
    char request_type;
    unsigned long request_addr;
    while (fscanf(trace, "%c\t0x%x\n", &request_type, &request_addr) != EOF) {
        printf("%c\t0x%x\n",request_type, request_addr);
        total_cnt++;
        char content[64];
        int read = 1;
        if (request_type == 'r') {
            read = 1;
        } else if (request_type == 'w') {
            read = 0;
        } else {
            printf("Error: invalid access type.\n");
            continue;
        }
        int hit = 0, time = 0;
        int bytes = 1; // TODO
        l1.HandleRequest(request_addr, bytes, read, content, hit, time, 1);
        total_hit += hit;
        success_cnt++;
    }
    fclose(trace);
    return;
}

CacheConfig config_cache(int size, int associativity, int block_size, int write_through, int write_allocate, int ev_tags_num) {
    CacheConfig cc;
    cc.size = size;
    cc.associativity = associativity;
    cc.block_size = block_size;
    cc.set_num = size / (associativity * block_size);
    cc.write_through = write_through;
    cc.write_allocate = write_allocate;
    cc.ev_tags_num = ev_tags_num;
    return cc;
}


