#pragma once
#include "Types.h"
#include "Tlb.h"
#include "PageTable.h"
#include "Cache.h"
#include <memory>
#include <vector>

struct AccessOutcome {
    bool tlbHit;
    bool pageFault;
    int hitLevel; // 0/1/2 = L1/L2/L3 hit, -1 = went to RAM
    double latencyCycles;
};

class MemoryHierarchy {
public:
    explicit MemoryHierarchy(const SimConfig& cfg);

    // trace/traceIdx are only used to give the OPTIMAL page-replacement
    // algorithm perfect future knowledge (offline analysis, as in Belady's
    // original algorithm). Pass nullptr/0 if unused.
    AccessOutcome access(Addr virtualAddr, long long time,
                          const std::vector<int>* pageTrace = nullptr, size_t traceIdx = 0);

    const SimStats& stats() const { return stats_; }

private:
    SimConfig cfg_;
    Tlb tlb_;
    PageTable pageTable_;
    std::vector<std::unique_ptr<CacheLevel>> levels_; // L1, L2, L3
    SimStats stats_;
};
