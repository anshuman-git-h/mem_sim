#include "WorkloadGenerator.h"
#include <random>

std::vector<Addr> generateWorkload(const WorkloadParams& p, std::vector<int>* outPageTrace) {
    std::vector<Addr> trace;
    trace.reserve(p.numAccesses);
    std::mt19937 rng(p.seed);

    switch (p.type) {
        case WorkloadType::SEQUENTIAL: {
            int page = 0;
            for (int i = 0; i < p.numAccesses; ++i) {
                trace.push_back((Addr)page * p.pageSizeBytes);
                page = (page + p.strideByPages) % p.virtualPages;
            }
            break;
        }
        case WorkloadType::RANDOM: {
            std::uniform_int_distribution<int> dist(0, p.virtualPages - 1);
            for (int i = 0; i < p.numAccesses; ++i)
                trace.push_back((Addr)dist(rng) * p.pageSizeBytes);
            break;
        }
        case WorkloadType::LOOPING: {
            // Repeatedly scans a fixed window of pages -> stresses whether a
            // policy can "recognize" a cyclic pattern (classic FIFO/LRU
            // pathological case, e.g. Belady's anomaly territory).
            int window = std::min(p.loopWindowPages, p.virtualPages);
            for (int i = 0; i < p.numAccesses; ++i)
                trace.push_back((Addr)(i % window) * p.pageSizeBytes);
            break;
        }
        case WorkloadType::WORKING_SET: {
            // Most references land in a small "hot" set (temporal locality);
            // occasionally jumps to a cold page elsewhere in the address space.
            int hotSize = std::min(p.workingSetPages, p.virtualPages);
            std::uniform_int_distribution<int> hotDist(0, hotSize - 1);
            std::uniform_int_distribution<int> coldDist(0, p.virtualPages - 1);
            std::uniform_real_distribution<double> coin(0.0, 1.0);
            for (int i = 0; i < p.numAccesses; ++i) {
                int page = (coin(rng) < p.workingSetLocality) ? hotDist(rng) : coldDist(rng);
                trace.push_back((Addr)page * p.pageSizeBytes);
            }
            break;
        }
    }

    if (outPageTrace) {
        outPageTrace->clear();
        outPageTrace->reserve(trace.size());
        for (Addr a : trace) outPageTrace->push_back((int)(a / p.pageSizeBytes));
    }
    return trace;
}
