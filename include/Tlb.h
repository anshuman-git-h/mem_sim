#pragma once
#include <unordered_map>
#include <list>

// Fully-associative TLB with LRU replacement (TLBs are small & highly
// associative in real hardware, so LRU-on-a-small-set is a realistic model).
class Tlb {
public:
    explicit Tlb(int capacity);

    bool lookup(int vpage, int& outFrame);
    void insert(int vpage, int frame);
    void invalidate(int vpage);

private:
    int capacity_;
    std::list<int> lru_; // front = most recently used
    std::unordered_map<int, std::pair<int, std::list<int>::iterator>> map_;
};
