#pragma once
#include "Types.h"
#include <vector>
#include <deque>

struct CacheLine {
    bool valid = false;
    uint64_t tag = 0;
    long long lastUsed = 0;
};

// One level of a set-associative cache. The hierarchy composes several of
// these (L1/L2/L3) in MemoryHierarchy.
class CacheLevel {
public:
    CacheLevel(CacheLevelConfig cfg, CacheReplacement policy);

    // Returns true on hit and updates replacement metadata for that line.
    bool access(Addr physAddr, long long time);

    // Called after a miss propagates to a lower level / RAM; installs the
    // line in this level, evicting per the configured policy if needed.
    // Returns true if an eviction of a valid line occurred.
    bool insertLine(Addr physAddr, long long time);

    int numSets() const { return numSets_; }
    int associativity() const { return assoc_; }

private:
    int lineSizeBytes_;
    int assoc_;
    int numSets_;
    CacheReplacement policy_;

    std::vector<std::vector<CacheLine>> sets_;
    std::vector<std::deque<int>> fifoOrder_;     // per-set way insertion order
    std::vector<std::vector<int>> plruBits_;     // per-set tree bits (assoc-1 of them)

    int offsetBits_;
    int indexBits_;

    int setIndexOf(Addr addr) const;
    uint64_t tagOf(Addr addr) const;

    int chooseVictimWay(int setIdx, long long time);
    void touchPLRU(int setIdx, int way);
};
