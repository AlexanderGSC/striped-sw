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
    const double seconds = d.count();
    const double cups  = total_cells / seconds;
    const double mcups = cups / 1e6;
    const double gcups = mcups / 1000.0; 

    std::cout << "\n=== STRIP SMITH-WATERMAN RISC-V RVV (LMUL=" << CONFIG_LMUL << ") ===\n"
              << "Alignment: I=" << std::get<0>(r) 
              << " J=" << std::get<1>(r) 
              << " SCORE=" << std::get<2>(r) << "\n"
              << "Execution Time: " << seconds << " s (" << seconds * 1000.0 << " ms)\n"
              << "Throughput:     " << mcups << " MCUPS (" << gcups << " GCUPS)\n"
              << "Cells/Cycle:    " << total_cells / (double)cycles.get() << "\n"
              << "Cells/Inst:     " << total_cells / (double)instructions.get() << "\n"
              << std::endl;
    
    std::cout << "========================================\n";
    std::cout << "  PERF STATS (RISC-V RVV LMUL=" << CONFIG_LMUL << ")\n";
    std::cout << "========================================\n";
    
    // Co4 decimales
    std::cout << std::fixed << std::setprecision(4);

    // Row1: Cycles, Instructions and IPC
    std::cout << std::left  << std::setw(18) << "Cycles:"         << std::right << std::setw(15) << cycles.get() << "\n"
              << std::left  << std::setw(18) << "Instructions:"   << std::right << std::setw(15) << instructions.get() << "\n";
    if (cycles.get() > 0) {
        std::cout << std::left  << std::setw(18) << "IPC:"          << std::right << std::setw(14) << (double)instructions.get() / cycles.get() << "\n";
    }
    std::cout << "----------------------------------------\n";

    // Fila 2: Caché L1
    std::cout << std::left  << std::setw(18) << "L1 Loads:"       << std::right << std::setw(15) << l1_loads.get() << "\n"
              << std::left  << std::setw(18) << "L1 Misses:"      << std::right << std::setw(15) << l1_misses.get() << "\n";
    if (l1_loads.get() > 0) {
        std::cout << std::left  << std::setw(18) << "L1 Miss Rate:"  << std::right << std::setw(14) << ((double)l1_misses.get() / l1_loads.get() * 100.0) << "%\n";
    }
    std::cout << "----------------------------------------\n";

    // Fila 3: Stalls (Paradas de Pipeline)
    std::cout << std::left  << std::setw(18) << "Frontend Stalls:" << std::right << std::setw(15) << frontend_stalls.get() << "\n"
              << std::left  << std::setw(18) << "Backend Stalls:"  << std::right << std::setw(15) << backend_stalls.get() << "\n";
    if (cycles.get() > 0) {
        std::cout << std::left  << std::setw(18) << "Stalls Rate:"    << std::right << std::setw(14) << ((double)backend_stalls.get() / cycles.get() * 100.0) << "%\n";
    }
    std::cout << "----------------------------------------\n";

    // Fila 4: Predicción de Saltos
    std::cout << std::left  << std::setw(18) << "Branches:"       << std::right << std::setw(15) << branches.get() << "\n"
              << std::left  << std::setw(18) << "Branch Misses:"  << std::right << std::setw(15) << branch_misses.get() << "\n";
    if (branches.get() > 0) {
        std::cout << std::left  << std::setw(18) << "Branch Miss %:"  << std::right << std::setw(14) << ((double)branch_misses.get() / branches.get() * 100.0) << "%\n";
    }
    std::cout << "========================================\n" << std::endl;

    
    return EXIT_SUCCESS;
}