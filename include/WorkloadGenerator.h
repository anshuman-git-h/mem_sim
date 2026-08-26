#pragma once
#include "Types.h"
#include <vector>

struct WorkloadParams {
    WorkloadType type = WorkloadType::RANDOM;
    int numAccesses   = 100000;
    int virtualPages  = 4096;
    int strideByPages = 1;      // SEQUENTIAL
    int loopWindowPages = 32;   // LOOPING: pages repeatedly re-referenced
    int workingSetPages = 64;   // WORKING_SET: hot set size
    double workingSetLocality = 0.9; // prob. of hitting inside the hot set
    int pageSizeBytes = 4096;
    unsigned seed = 42;
};

// Generates a trace of *virtual byte addresses*. A parallel page-number
// trace (address / pageSizeBytes) is what the OPTIMAL algorithm looks ahead
// into, so it's produced too for convenience.
std::vector<Addr> generateWorkload(const WorkloadParams& p, std::vector<int>* outPageTrace = nullptr);
