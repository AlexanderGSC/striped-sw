#pragma once

#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <cstdint>
#include <iomanip>


namespace perf{

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


class PerfProfile {

    public:
    PerfCounter cycles, instructions, l1_loads, l1_misses,
        frontend_stalls, backend_stalls, branches, branch_misses; 
    PerfProfile() :
        cycles(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES),
        instructions(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS),
        l1_loads(PERF_TYPE_HW_CACHE, 
                         PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8)),
        l1_misses(PERF_TYPE_HW_CACHE,
                         PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)),
        frontend_stalls(PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND),
        backend_stalls(PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND), 
        branches(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS),
        branch_misses(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES)
    {}

    void stop() {
        cycles.stop();
        instructions.stop();
        l1_loads.stop();
        l1_misses.stop();
        frontend_stalls.stop();
        backend_stalls.stop();
        branches.stop();
        branch_misses.stop();
    }

    double getBackendStallsRate() {return (double)backend_stalls.get() / cycles.get() * 100.0;}
    double getIPC() {return (double)instructions.get() / cycles.get();}
    double getBranchMissRate() {return (double)branch_misses.get() / branches.get() * 100.0;}
    double getL1MissRate() {return (double)l1_misses.get() / l1_loads.get() * 100.0;}
};

}