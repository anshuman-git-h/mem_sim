#include "Tlb.h"

Tlb::Tlb(int capacity) : capacity_(capacity) {}

bool Tlb::lookup(int vpage, int& outFrame) {
    auto it = map_.find(vpage);
    if (it == map_.end()) return false;
    outFrame = it->second.first;
    lru_.erase(it->second.second);
    lru_.push_front(vpage);
    it->second.second = lru_.begin();
    return true;
}

void Tlb::insert(int vpage, int frame) {
    auto it = map_.find(vpage);
    if (it != map_.end()) {
        lru_.erase(it->second.second);
        lru_.push_front(vpage);
        it->second = {frame, lru_.begin()};
        return;
    }
    if ((int)map_.size() >= capacity_) {
        int victim = lru_.back();
        lru_.pop_back();
        map_.erase(victim);
    }
    lru_.push_front(vpage);
    map_[vpage] = {frame, lru_.begin()};
}

void Tlb::invalidate(int vpage) {
    auto it = map_.find(vpage);
    if (it == map_.end()) return;
    lru_.erase(it->second.second);
    map_.erase(it);
}
