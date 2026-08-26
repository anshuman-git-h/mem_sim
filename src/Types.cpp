#include "Types.h"

std::string toString(PageReplacement p) {
    switch (p) {
        case PageReplacement::FIFO: return "FIFO";
        case PageReplacement::LRU: return "LRU";
        case PageReplacement::OPTIMAL: return "OPTIMAL";
        case PageReplacement::CLOCK: return "CLOCK";
        case PageReplacement::LFU: return "LFU";
    }
    return "?";
}

std::string toString(CacheReplacement c) {
    switch (c) {
        case CacheReplacement::LRU: return "LRU";
        case CacheReplacement::FIFO: return "FIFO";
        case CacheReplacement::RANDOM: return "RANDOM";
        case CacheReplacement::PSEUDO_LRU: return "PLRU";
    }
    return "?";
}

std::string toString(WorkloadType w) {
    switch (w) {
        case WorkloadType::SEQUENTIAL: return "SEQUENTIAL";
        case WorkloadType::RANDOM: return "RANDOM";
        case WorkloadType::LOOPING: return "LOOPING";
        case WorkloadType::WORKING_SET: return "WORKING_SET";
    }
    return "?";
}
