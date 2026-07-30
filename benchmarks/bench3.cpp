#include <ssw/perf.hpp>
#include <ssw/rvv-ssw.hpp>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: bench3 <database size> <query size>" << std::endl;
        return EXIT_SUCCESS;
    }

    size_t database_length = static_cast<size_t>(atoi(argv[1]));
    size_t query_length    = static_cast<size_t>(atoi(argv[2]));
    size_t seed = 0; 
    ssw::Sequence database = ssw::generate_random_sequence(database_length, seed);
    ssw::Sequence query    = ssw::generate_random_sequence(query_length, seed);

    std::cout << "QUERY LENGTH=" << std::setw(4) << query.size() << std::endl;
    std::cout << "DATABASE LENGTH=" << std::setw(4) << database.size() << std::endl;
    
    const long long total_cells = static_cast<long long>(database_length) 
                * static_cast<long long>(query_length);

    // LMUL = 1
    // ------------------------------------------------
    perf::PerfProfile perf_profile1 {};
    const auto start1 = std::chrono::steady_clock::now();
    Result r1 = riscv_ssw::strip_smith_waterman<1>(query, database);   
    const auto finish1 = std::chrono::steady_clock::now();
    perf_profile1.stop();

    const std::chrono::duration<double> d1 = finish1 - start1;

    auto gcups1 = total_cells / d1.count() / 1e9;
    auto cups_cycle1 = total_cells / perf_profile1.cycles.get();
    auto cups_instr1  = total_cells / perf_profile1.instructions.get();
    // GCUPS + CUPS / cycle + CUPS / intructions
    // IPC %L1 MISS RATE %BACKEND STALLS

    std::cout << "GCUPS      " << std::setw(8) << gcups1 <<
                 "CUPS/CYCLE " << std::setw(8) << cups_cycle1 <<
                 "CUPS/INSTR " << std::setw(8) << cups_instr1 << "\n";

    std::cout << "IPC        " << std::setw(8) << perf_profile1.getIPC() <<
                 "% L1 MISS  " << std::setw(8) << perf_profile1.getL1MissRate() << "%" <<
                 "STALLS     " << std::setw(8) << perf_profile1.getBackendStallsRate() << "%\n";
    //------------------------------------------------------

    //LMUL = 2
    perf::PerfProfile perf_profile2 {};
    const auto start2 = std::chrono::steady_clock::now();
    Result r2 = riscv_ssw::strip_smith_waterman<2>(query, database);   
    const auto finish2 = std::chrono::steady_clock::now();
    perf_profile2.stop();

    perf::PerfProfile perf_profile4 {};
    const auto start4 = std::chrono::steady_clock::now();
    Result r4 = riscv_ssw::strip_smith_waterman<4>(query, database);   
    const auto finish4 = std::chrono::steady_clock::now();
    perf_profile4.stop();   
    
    perf::PerfProfile perf_profile8 {};
    const auto start8 = std::chrono::steady_clock::now();
    Result r8 = riscv_ssw::strip_smith_waterman<8>(query, database);   
    const auto finish8 = std::chrono::steady_clock::now();
    perf_profile8.stop();
    
    
    return EXIT_SUCCESS;
}