#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <ssw/rvv-ssw.hpp>

#ifndef CONFIG_LMUL 
#define CONFIG_LMUL 1
#endif

class PerfCounter {
    int fd = -1;
    long long value = 0;
    bool stopped = false;

public:
    PerfCounter(uint32_t event_type, uint32_t event_config) {
        struct perf_event_attr pe = {};
        pe.type = event_type;
        pe.size = sizeof(pe);
        pe.config = event_config;
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;
        
        fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
        if (fd == -1) throw std::runtime_error("perf_event_open failed");
        
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }

    void stop() {
        if (fd != -1 && !stopped) {
            ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
            if (read(fd, &value, sizeof(value)) < 0) {
                value = 0;
            }
            stopped = true;
        }
    }
    
    ~PerfCounter() {
        if (fd != -1) {
            if (!stopped) stop();
            close(fd);
        }
    }
    
    long long get() const { return value; }
};

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cout << "Usage: bench2 <database size> <query size>" << std::endl;
        return EXIT_SUCCESS;
    }

    size_t database_length = static_cast<size_t>(atoi(argv[1]));
    size_t query_length    = static_cast<size_t>(atoi(argv[2]));
    size_t seed = 0; 
    Sequence database = ssw::generate_random_sequence(database_length, seed);
    Sequence query    = ssw::generate_random_sequence(query_length, seed);

    std::cout << "QUERY LENGTH=" << std::setw(4) << query.size() << std::endl;
    std::cout << "DATABASE LENGTH=" << std::setw(4) << database.size() << std::endl; 

    PerfCounter cycles(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    PerfCounter instructions(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    PerfCounter l1_loads(PERF_TYPE_HW_CACHE, 
                         PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8));
    PerfCounter l1_misses(PERF_TYPE_HW_CACHE,
                          PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16));
    
    PerfCounter frontend_stalls(PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND);
    PerfCounter backend_stalls(PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND);
    
    
    PerfCounter branches(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
    PerfCounter branch_misses(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);

    const auto start = std::chrono::steady_clock::now();
    
    // Kernel execution
    Result r = riscv_ssw::strip_smith_waterman<CONFIG_LMUL>(query, database);
    
    const auto finish = std::chrono::steady_clock::now();

    // Stop counters
    cycles.stop();
    instructions.stop();
    l1_loads.stop();
    l1_misses.stop();
    frontend_stalls.stop();
    backend_stalls.stop();
    branches.stop();
    branch_misses.stop();

    #if defined(__GNUC__) || defined(__CLANG__)
    asm volatile("" : : "r"(&r) : "memory");
    #endif

    const std::chrono::duration<double> d = finish - start;
    const double total_cells = static_cast<double>(query_length) * static_cast<double>(database_length);
    const double seconds1 = d.count();
    const double mcups1 = (total_cells / seconds1) / 1e6;
    const double gcups1 = mcups1 / 1000.0; 

    std::cout << "\n--- STRIP SMITH-WATERMAN RISC-V RVV (LMUL=" << CONFIG_LMUL << ") ---\n"
              << "Alignment: I=" << std::get<0>(r) 
              << " J=" << std::get<1>(r) 
              << " SCORE=" << std::get<2>(r) << "\n"
              << "Execution Time: " << seconds1 << " s (" << seconds1 * 1000.0 << " ms)\n"
              << "Throughput:     " << mcups1 << " MCUPS (" << gcups1 << " GCUPS)\n" 
              << std::endl;
    
    std::cout << "--- Perf Stats for LMUL=" << CONFIG_LMUL << " ---\n";
    std::cout << "Cycles:          " << cycles.get() << "\n";
    std::cout << "Instructions:    " << instructions.get() << "\n";
    if (cycles.get() > 0) {
        std::cout << "IPC:             " << (double)instructions.get() / cycles.get() << "\n";
    }
    std::cout << "L1 Loads:        " << l1_loads.get() << "\n";
    std::cout << "L1 Misses:       " << l1_misses.get() << "\n";
    if (l1_loads.get() > 0) {
        std::cout << "Miss Rate:       " << (double)l1_misses.get() / l1_loads.get() * 100.0 << "%\n";
    }
    std::cout << "Frontend Stalls: " << frontend_stalls.get() << "\n";
    std::cout << "Backend Stalls:  " << backend_stalls.get() << "\n";
    std::cout << "Stalls Rate:     " << (double)backend_stalls.get() / cycles.get() * 100.0 << "%\n";
    std::cout << "Branches:        " << branches.get() << "\n";
    std::cout << "Branch Misses:   " << branch_misses.get() << "\n";
    if (branches.get() > 0) {
        std::cout << "Branch Miss Rate:" << (double)branch_misses.get() / branches.get() * 100.0 << "%\n";
    }
    
    return EXIT_SUCCESS;
}