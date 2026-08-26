#pragma once
#include <cstdint>
#include <array>
#include <string>

using Addr = uint64_t;

enum class PageReplacement { FIFO, LRU, OPTIMAL, CLOCK, LFU };
enum class CacheReplacement { LRU, FIFO, RANDOM, PSEUDO_LRU };
enum class WorkloadType { SEQUENTIAL, RANDOM, LOOPING, WORKING_SET };

std::string toString(PageReplacement p);
std::string toString(CacheReplacement c);
std::string toString(WorkloadType w);

struct CacheLevelConfig {
    int sizeBytes;
    int lineSizeBytes;
    int associativity;   // must be a power of two for PSEUDO_LRU
    int latencyCycles;
};

struct SimConfig {
    // Virtual memory
    int virtualPages     = 4096;   // size of virtual address space, in pages
    int physicalFrames   = 256;    // size of physical RAM, in frames
    int pageSizeBytes    = 4096;
    int tlbEntries       = 64;
    PageReplacement pageAlgo = PageReplacement::LRU;

    // Cache hierarchy
    CacheLevelConfig l1{32 * 1024,   64, 8,  4};
    CacheLevelConfig l2{256 * 1024,  64, 8,  12};
    CacheLevelConfig l3{8 * 1024 * 1024, 64, 16, 40};
    CacheReplacement cacheAlgo = CacheReplacement::LRU;

    // Latencies (cycles)
    int tlbLatencyCycles           = 1;
    int pageTableWalkLatencyCycles = 100;
    int ramLatencyCycles           = 200;
    int diskLatencyCycles          = 200000; // page fault service time
};

struct SimStats {
    long long totalAccesses  = 0;
    long long tlbHits        = 0;
    long long tlbMisses      = 0;
    long long pageFaults     = 0;
    long long pageEvictions  = 0;
    std::array<long long, 3> cacheHits{};
    std::array<long long, 3> cacheMisses{};
    long long cacheEvictions = 0;
    double totalLatencyCycles = 0.0;

    double amat() const { return totalAccesses ? totalLatencyCycles / (double)totalAccesses : 0.0; }
    double tlbHitRate() const { return totalAccesses ? (double)tlbHits / (double)totalAccesses : 0.0; }
    double pageFaultRate() const { return totalAccesses ? (double)pageFaults / (double)totalAccesses : 0.0; }
    double cacheHitRate(int level) const {
        long long h = cacheHits[level], m = cacheMisses[level];
        return (h + m) ? (double)h / (double)(h + m) : 0.0;
    }
};
