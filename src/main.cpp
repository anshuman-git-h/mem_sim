#include "Types.h"
#include "MemoryHierarchy.h"
#include "WorkloadGenerator.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

struct RunResult {
    PageReplacement pageAlgo;
    CacheReplacement cacheAlgo;
    WorkloadType workload;
    SimStats stats;
};

static RunResult runOne(PageReplacement pageAlgo, CacheReplacement cacheAlgo,
                         WorkloadType workload, int numAccesses) {
    SimConfig cfg;
    cfg.pageAlgo = pageAlgo;
    cfg.cacheAlgo = cacheAlgo;
    cfg.virtualPages = 4096;
    cfg.physicalFrames = 256; // 16x oversubscribed virtual->physical -> real faulting pressure

    WorkloadParams wp;
    wp.type = workload;
    wp.numAccesses = numAccesses;
    wp.virtualPages = cfg.virtualPages;
    wp.pageSizeBytes = cfg.pageSizeBytes;

    std::vector<int> pageTrace;
    std::vector<Addr> trace = generateWorkload(wp, &pageTrace);

    MemoryHierarchy mem(cfg);
    for (size_t i = 0; i < trace.size(); ++i) {
        mem.access(trace[i], (long long)i, &pageTrace, i);
    }

    return {pageAlgo, cacheAlgo, workload, mem.stats()};
}

static void printRow(const RunResult& r) {
    std::cout << std::left
               << std::setw(9)  << toString(r.pageAlgo)
               << std::setw(7)  << toString(r.cacheAlgo)
               << std::setw(13) << toString(r.workload)
               << std::right
               << std::setw(10) << std::fixed << std::setprecision(2) << (r.stats.tlbHitRate() * 100) << "%"
               << std::setw(10) << (r.stats.pageFaultRate() * 100) << "%"
               << std::setw(10) << (r.stats.cacheHitRate(0) * 100) << "%"
               << std::setw(10) << (r.stats.cacheHitRate(1) * 100) << "%"
               << std::setw(10) << (r.stats.cacheHitRate(2) * 100) << "%"
               << std::setw(12) << r.stats.amat()
               << "\n";
}

int main(int argc, char** argv) {
    int numAccesses = 200000;
    if (argc > 1) numAccesses = std::atoi(argv[1]);

    std::vector<PageReplacement> pageAlgos = {
        PageReplacement::FIFO, PageReplacement::LRU, PageReplacement::OPTIMAL,
        PageReplacement::CLOCK, PageReplacement::LFU
    };
    std::vector<CacheReplacement> cacheAlgos = {
        CacheReplacement::LRU, CacheReplacement::FIFO,
        CacheReplacement::RANDOM, CacheReplacement::PSEUDO_LRU
    };
    std::vector<WorkloadType> workloads = {
        WorkloadType::SEQUENTIAL, WorkloadType::RANDOM,
        WorkloadType::LOOPING, WorkloadType::WORKING_SET
    };

    std::vector<RunResult> results;

    std::cout << std::left
               << std::setw(9) << "PageAlg" << std::setw(7) << "CacheA" << std::setw(13) << "Workload"
               << std::right
               << std::setw(11) << "TLB-Hit" << std::setw(11) << "PgFault" << std::setw(11) << "L1-Hit"
               << std::setw(10) << "L2-Hit" << std::setw(10) << "L3-Hit" << std::setw(12) << "AMAT(cyc)"
               << "\n";
    std::cout << std::string(104, '-') << "\n";

    // Full sweep: 5 page algos x 4 cache algos x 4 workloads = 80 configurations.
    for (auto pa : pageAlgos) {
        for (auto ca : cacheAlgos) {
            for (auto wl : workloads) {
                RunResult r = runOne(pa, ca, wl, numAccesses);
                printRow(r);
                results.push_back(r);
            }
        }
    }

    std::ofstream csv("results.csv");
    csv << "page_algo,cache_algo,workload,tlb_hit_rate,page_fault_rate,"
           "l1_hit_rate,l2_hit_rate,l3_hit_rate,amat_cycles,page_evictions,cache_evictions\n";
    for (auto& r : results) {
        csv << toString(r.pageAlgo) << "," << toString(r.cacheAlgo) << "," << toString(r.workload) << ","
            << r.stats.tlbHitRate() << "," << r.stats.pageFaultRate() << ","
            << r.stats.cacheHitRate(0) << "," << r.stats.cacheHitRate(1) << "," << r.stats.cacheHitRate(2) << ","
            << r.stats.amat() << "," << r.stats.pageEvictions << "," << r.stats.cacheEvictions << "\n";
    }
    std::cout << "\nWrote " << results.size() << " configurations to results.csv\n";
    return 0;
}
