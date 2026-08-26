#include "PageTable.h"
#include <algorithm>
#include <limits>

PageTable::PageTable(int virtualPages, int physicalFrames, PageReplacement algo)
    : numFrames_(physicalFrames), algo_(algo) {
    (void)virtualPages;
    frameToVpage_.assign(physicalFrames, -1);
    clockRefBit_.assign(physicalFrames, 0);
    for (int f = physicalFrames - 1; f >= 0; --f) freeFrames_.push_back(f);
}

int PageTable::translate(int vpage) const {
    auto it = vpageToFrame_.find(vpage);
    return it == vpageToFrame_.end() ? -1 : it->second;
}

void PageTable::recordAccess(int vpage, long long time) {
    switch (algo_) {
        case PageReplacement::LRU: {
            auto it = lruIter_.find(vpage);
            if (it != lruIter_.end()) {
                lruList_.erase(it->second);
                lruList_.push_front(vpage);
                it->second = lruList_.begin();
            }
            break;
        }
        case PageReplacement::CLOCK: {
            auto it = vpageToFrame_.find(vpage);
            if (it != vpageToFrame_.end()) clockRefBit_[it->second] = 1;
            break;
        }
        case PageReplacement::LFU:
            freq_[vpage]++;
            break;
        case PageReplacement::FIFO:
        case PageReplacement::OPTIMAL:
        default:
            break; // no bookkeeping needed on plain hits
    }
    (void)time;
}

int PageTable::allocFrame(int vpage) {
    int frame = freeFrames_.back();
    freeFrames_.pop_back();
    frameToVpage_[frame] = vpage;
    return frame;
}

int PageTable::chooseVictimFrame(const std::vector<int>* trace, size_t traceIdx) {
    switch (algo_) {
        case PageReplacement::FIFO: {
            int victimVpage = fifoQueue_.front();
            fifoQueue_.pop_front();
            return vpageToFrame_[victimVpage];
        }
        case PageReplacement::LRU: {
            int victimVpage = lruList_.back();
            lruList_.pop_back();
            lruIter_.erase(victimVpage);
            return vpageToFrame_[victimVpage];
        }
        case PageReplacement::LFU: {
            long long best = std::numeric_limits<long long>::max();
            int victimVpage = -1;
            for (auto& kv : vpageToFrame_) {
                long long f = freq_.count(kv.first) ? freq_[kv.first] : 0;
                if (f < best) { best = f; victimVpage = kv.first; }
            }
            freq_.erase(victimVpage);
            return vpageToFrame_[victimVpage];
        }
        case PageReplacement::CLOCK: {
            while (true) {
                int frame = clockHand_;
                clockHand_ = (clockHand_ + 1) % numFrames_;
                if (clockRefBit_[frame] == 0) return frame;
                clockRefBit_[frame] = 0;
            }
        }
        case PageReplacement::OPTIMAL: {
            // Belady's algorithm: evict the resident page whose next use is
            // furthest in the future (or never used again).
            int victimVpage = -1;
            size_t farthest = 0;
            bool farthestIsInfinite = false;
            for (auto& kv : vpageToFrame_) {
                int vp = kv.first;
                size_t nextUse = std::numeric_limits<size_t>::max();
                bool usedAgain = false;
                if (trace) {
                    for (size_t i = traceIdx; i < trace->size(); ++i) {
                        if ((*trace)[i] == vp) { nextUse = i; usedAgain = true; break; }
                    }
                }
                if (!usedAgain) {
                    victimVpage = vp;
                    farthestIsInfinite = true;
                    break; // a page never used again is always a valid choice
                }
                if (!farthestIsInfinite && nextUse > farthest) {
                    farthest = nextUse;
                    victimVpage = vp;
                }
            }
            return vpageToFrame_[victimVpage];
        }
    }
    return -1;
}

void PageTable::onLoad(int vpage, int frame, long long time) {
    vpageToFrame_[vpage] = frame;
    frameToVpage_[frame] = vpage;
    switch (algo_) {
        case PageReplacement::FIFO:
            fifoQueue_.push_back(vpage);
            break;
        case PageReplacement::LRU:
            lruList_.push_front(vpage);
            lruIter_[vpage] = lruList_.begin();
            break;
        case PageReplacement::CLOCK:
            clockRefBit_[frame] = 1;
            break;
        case PageReplacement::LFU:
            freq_[vpage] = 1;
            break;
        case PageReplacement::OPTIMAL:
            break;
    }
    (void)time;
}

void PageTable::onEvict(int vpage, int frame) {
    vpageToFrame_.erase(vpage);
    frameToVpage_[frame] = -1;
}

PageTable::FaultResult PageTable::handleFault(int vpage, long long time,
                                               const std::vector<int>* trace, size_t traceIdx) {
    if (!freeFrames_.empty()) {
        int frame = allocFrame(vpage);
        onLoad(vpage, frame, time);
        return {frame, -1};
    }
    int victimFrame = chooseVictimFrame(trace, traceIdx);
    int victimVpage = frameToVpage_[victimFrame];
    onEvict(victimVpage, victimFrame);
    onLoad(vpage, victimFrame, time);
    return {victimFrame, victimVpage};
}
