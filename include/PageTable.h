#pragma once
#include "Types.h"
#include <vector>
#include <deque>
#include <list>
#include <unordered_map>

// Manages the virtual-page -> physical-frame mapping and implements the
// page replacement algorithm used when a page fault occurs and RAM is full.
class PageTable {
public:
    PageTable(int virtualPages, int physicalFrames, PageReplacement algo);

    // Returns the physical frame for vpage, or -1 if not resident.
    int translate(int vpage) const;

    // Called on every resident-page access (TLB miss but page-table hit) so
    // recency/frequency/reference bits stay accurate for LRU/LFU/Clock.
    void recordAccess(int vpage, long long time);

    struct FaultResult { int frame; int evictedVPage; };

    // Called on a page fault. `trace`/`traceIdx` give the simulator's full
    // future reference stream so the OPTIMAL (Belady) algorithm can look
    // ahead; other algorithms ignore these parameters.
    FaultResult handleFault(int vpage, long long time,
                             const std::vector<int>* trace, size_t traceIdx);

private:
    int numFrames_;
    PageReplacement algo_;

    std::unordered_map<int, int> vpageToFrame_;
    std::vector<int> frameToVpage_;      // -1 if frame is free
    std::vector<int> freeFrames_;        // stack of unused frame ids

    // --- FIFO ---
    std::deque<int> fifoQueue_;

    // --- LRU ---
    std::list<int> lruList_;             // front = MRU
    std::unordered_map<int, std::list<int>::iterator> lruIter_;

    // --- CLOCK (second chance) ---
    std::vector<int> clockRefBit_;       // indexed by frame
    int clockHand_ = 0;

    // --- LFU ---
    std::unordered_map<int, long long> freq_;

    int allocFrame(int vpage);
    int chooseVictimFrame(const std::vector<int>* trace, size_t traceIdx);
    void onLoad(int vpage, int frame, long long time);
    void onEvict(int vpage, int frame);
};
