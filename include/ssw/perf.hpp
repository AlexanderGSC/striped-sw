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
    long long scaled_value = 0;
    double multiplex_ratio = 1.0; // 1.0 = 100% Exacto (Sin multiplexado)
    bool stopped = false;

    // Estructura de lectura cuando activamos los flags de tiempo
    struct perf_read_format {
        uint64_t value;        // Cuenta bruta del HW
        uint64_t time_enabled; // Tiempo total que el contador estuvo activo
        uint64_t time_running; // Tiempo real que estuvo mapeado en un registro HW
    };

public:
    PerfCounter(uint32_t event_type, uint64_t event_config) {
        struct perf_event_attr pe = {};
        pe.type = event_type;
        pe.size = sizeof(pe);
        pe.config = event_config;
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;

        // CRÍTICO: Pedir los tiempos para detectar y corregir multiplexado
        pe.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

        fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
        if (fd == -1) {
            throw std::runtime_error("perf_event_open failed (¿demasiados eventos o evento no soportado?)");
        }

        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }

    void stop() {
        if (fd != -1 && !stopped) {
            ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

            perf_read_format data{};
            if (read(fd, &data, sizeof(data)) == sizeof(data)) {
                if (data.time_running > 0) {
                    // 1. Escalar el valor según el tiempo real ejecutado
                    scaled_value = static_cast<long long>(
                        static_cast<double>(data.value) * static_cast<double>(data.time_enabled) / static_cast<double>(data.time_running)
                    );
                    
                    // 2. Calcular el ratio de precisión (1.0 = 100% de tiempo en HW)
                    multiplex_ratio = static_cast<double>(data.time_running) / static_cast<double>(data.time_enabled);
                } else {
                    scaled_value = 0;
                    multiplex_ratio = 0.0;
                }
            } else {
                scaled_value = 0;
                multiplex_ratio = 0.0;
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

    // Devuelve el valor estimado/escalado final
    long long get() const { return scaled_value; }

    // Devuelve el % de tiempo que el contador tuvo un registro físico dedicado
    double get_coverage() const { return multiplex_ratio; }

    //
    bool is_exact() const { return multiplex_ratio >= 0.9999; }
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