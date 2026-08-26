#include "MemoryHierarchy.h"

MemoryHierarchy::MemoryHierarchy(const SimConfig& cfg)
    : cfg_(cfg),
      tlb_(cfg.tlbEntries),
      pageTable_(cfg.virtualPages, cfg.physicalFrames, cfg.pageAlgo) {
    levels_.push_back(std::make_unique<CacheLevel>(cfg.l1, cfg.cacheAlgo));
    levels_.push_back(std::make_unique<CacheLevel>(cfg.l2, cfg.cacheAlgo));
    levels_.push_back(std::make_unique<CacheLevel>(cfg.l3, cfg.cacheAlgo));
}

AccessOutcome MemoryHierarchy::access(Addr virtualAddr, long long time,
                                       const std::vector<int>* pageTrace, size_t traceIdx) {
    int vpage = (int)(virtualAddr / cfg_.pageSizeBytes);
    uint64_t offset = virtualAddr % cfg_.pageSizeBytes;

    double latency = cfg_.tlbLatencyCycles;
    int frame;
    bool tlbHit = tlb_.lookup(vpage, frame);
    bool pageFault = false;

    if (tlbHit) {
        stats_.tlbHits++;
        pageTable_.recordAccess(vpage, time);
    } else {
        stats_.tlbMisses++;
        latency += cfg_.pageTableWalkLatencyCycles;
        frame = pageTable_.translate(vpage);
        if (frame == -1) {
            pageFault = true;
            stats_.pageFaults++;
            latency += cfg_.diskLatencyCycles;
            auto res = pageTable_.handleFault(vpage, time, pageTrace, traceIdx);
            frame = res.frame;
            if (res.evictedVPage != -1) {
                stats_.pageEvictions++;
                tlb_.invalidate(res.evictedVPage);
            }
        } else {
            pageTable_.recordAccess(vpage, time);
        }
        tlb_.insert(vpage, frame);
    }

    Addr physAddr = (Addr)frame * (Addr)cfg_.pageSizeBytes + offset;

    int hitLevel = -1;
    for (int lvl = 0; lvl < (int)levels_.size(); ++lvl) {
        latency += (lvl == 0 ? cfg_.l1.latencyCycles : lvl == 1 ? cfg_.l2.latencyCycles : cfg_.l3.latencyCycles);
        if (levels_[lvl]->access(physAddr, time)) {
            stats_.cacheHits[lvl]++;
            hitLevel = lvl;
            break;
        }
        stats_.cacheMisses[lvl]++;
    }

    if (hitLevel == -1) {
        latency += cfg_.ramLatencyCycles;
    }

    // Fill every level above the one that hit (or above RAM on a full miss),
    // modeling an inclusive fetch-on-miss hierarchy.
    int fillUpTo = (hitLevel == -1) ? (int)levels_.size() - 1 : hitLevel - 1;
    for (int lvl = 0; lvl <= fillUpTo; ++lvl) {
        if (levels_[lvl]->insertLine(physAddr, time)) stats_.cacheEvictions++;
    }

    stats_.totalAccesses++;
    stats_.totalLatencyCycles += latency;

    return {tlbHit, pageFault, hitLevel, latency};
}
