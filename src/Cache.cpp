#include "Cache.h"
#include <cmath>
#include <cstdlib>
#include <stdexcept>

static int log2i(int x) {
    int r = 0;
    while ((1 << r) < x) ++r;
    return r;
}

CacheLevel::CacheLevel(CacheLevelConfig cfg, CacheReplacement policy)
    : lineSizeBytes_(cfg.lineSizeBytes), assoc_(cfg.associativity), policy_(policy) {
    numSets_ = (cfg.sizeBytes / cfg.lineSizeBytes) / cfg.associativity;
    if (numSets_ < 1) numSets_ = 1;
    offsetBits_ = log2i(lineSizeBytes_);
    indexBits_ = log2i(numSets_);

    sets_.assign(numSets_, std::vector<CacheLine>(assoc_));
    fifoOrder_.assign(numSets_, std::deque<int>());
    if (policy_ == CacheReplacement::PSEUDO_LRU) {
        if ((assoc_ & (assoc_ - 1)) != 0)
            throw std::runtime_error("PSEUDO_LRU requires power-of-two associativity");
        plruBits_.assign(numSets_, std::vector<int>(assoc_ - 1, 0));
    }
}

int CacheLevel::setIndexOf(Addr addr) const {
    return (int)((addr >> offsetBits_) & (numSets_ - 1));
}

uint64_t CacheLevel::tagOf(Addr addr) const {
    return addr >> (offsetBits_ + indexBits_);
}

void CacheLevel::touchPLRU(int setIdx, int way) {
    int k = log2i(assoc_);
    auto& bits = plruBits_[setIdx];
    int node = 0;
    for (int level = 0; level < k; ++level) {
        int shift = k - 1 - level;
        int bit = (way >> shift) & 1;
        bits[node] = 1 - bit; // point away from the branch we just used
        node = 2 * node + 1 + bit;
    }
}

int CacheLevel::chooseVictimWay(int setIdx, long long time) {
    auto& set = sets_[setIdx];

    // Always prefer a free (invalid) way first.
    for (int w = 0; w < assoc_; ++w)
        if (!set[w].valid) return w;

    switch (policy_) {
        case CacheReplacement::LRU: {
            int victim = 0;
            long long oldest = set[0].lastUsed;
            for (int w = 1; w < assoc_; ++w)
                if (set[w].lastUsed < oldest) { oldest = set[w].lastUsed; victim = w; }
            return victim;
        }
        case CacheReplacement::FIFO: {
            int victim = fifoOrder_[setIdx].front();
            fifoOrder_[setIdx].pop_front();
            return victim;
        }
        case CacheReplacement::RANDOM:
            return std::rand() % assoc_;
        case CacheReplacement::PSEUDO_LRU: {
            int k = log2i(assoc_);
            auto& bits = plruBits_[setIdx];
            int node = 0, way = 0;
            for (int level = 0; level < k; ++level) {
                int bit = bits[node];
                way = (way << 1) | bit;
                node = 2 * node + 1 + bit;
            }
            return way;
        }
    }
    (void)time;
    return 0;
}

bool CacheLevel::access(Addr physAddr, long long time) {
    int setIdx = setIndexOf(physAddr);
    uint64_t tag = tagOf(physAddr);
    auto& set = sets_[setIdx];
    for (int w = 0; w < assoc_; ++w) {
        if (set[w].valid && set[w].tag == tag) {
            set[w].lastUsed = time;
            if (policy_ == CacheReplacement::PSEUDO_LRU) touchPLRU(setIdx, w);
            return true;
        }
    }
    return false;
}

bool CacheLevel::insertLine(Addr physAddr, long long time) {
    int setIdx = setIndexOf(physAddr);
    uint64_t tag = tagOf(physAddr);
    auto& set = sets_[setIdx];

    // Already present (can happen if a lower level filled it already)?
    for (int w = 0; w < assoc_; ++w)
        if (set[w].valid && set[w].tag == tag) { set[w].lastUsed = time; return false; }

    int way = chooseVictimWay(setIdx, time);
    bool evicted = set[way].valid;
    set[way] = CacheLine{true, tag, time};

    if (policy_ == CacheReplacement::FIFO) fifoOrder_[setIdx].push_back(way);
    if (policy_ == CacheReplacement::PSEUDO_LRU) touchPLRU(setIdx, way);

    return evicted;
}
