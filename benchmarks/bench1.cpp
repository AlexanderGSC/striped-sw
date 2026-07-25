#include <ssw/rvv-ssw.hpp>
#include <ssw/common.hpp>
#include <ssw/ssw.hpp>
#include <vector>
#include <tuple>
#include <string>
#include <algorithm>
#include <chrono>
#include <fstream>

// riscv64-linux-gnu-g++ -march=rv64gcv -mabi=lp64d -std=c++23 -Iinclude -static tests/test.cpp -o testv
void add_benchmark(const std::string filename,
            const std::vector<Result> r) {
    std::ofstream file(filename, std::ios::app);
    //auto clk = std::chrono::system_clock::now();
    //auto now = std::chrono::floor<std::chrono::milliseconds>(clk);
    if (file.is_open()) {
        //file << now << ' ';
        for (int i=0; i<r.size(); ++i) 
            file << std::get<0>(r[i]) << ' ' << 
                    std::get<1>(r[i]) << ' ' <<
                    std::get<2>(r[i]);
        file << std::endl;
        file.close();
    }
    else std::cerr << "Error opening file " << filename << std::endl;
}


int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cout << "Benchmark1: random database and query comparison." << std::endl;
        std::cout << "Usage: bench1 <database size> <query size>" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string filename = static_cast<std::string>("bench1_") + argv[1] + argv[2] + ".csv";
    std::vector<Result> r(2);
    size_t database_length = static_cast<size_t>(atoi(argv[1]));
    size_t query_length    = static_cast<size_t>(atoi(argv[2]));
    size_t seed = 0; //seed 0 means random values
    Sequence database = ssw::generate_random_sequence(database_length,seed);
    Sequence query    = ssw::generate_random_sequence(query_length,seed);

    std::cout << "QUERY LENGTH=" << std::setw(4) << query.size() << std::endl;
    std::cout << "DATABASE LENGTH=" << std::setw(4) << database.size() << std::endl; 

    const auto s1 = std::chrono::steady_clock::now();
    Result r1 = riscv_ssw::strip_smith_waterman(query, database);
    const auto e1 = std::chrono::steady_clock::now();

    // barrier avoiding compilers reordenation  
    #if defined(__GNUC__) || defined(__CLANG__)
    asm volatile("" : : "r"(&r1) : "memory");
    #endif


    const std::chrono::duration<double> d1 = e1 - s1;
    const double total_cells = static_cast<double>(query_length) * static_cast<double>(database_length);
    const double seconds1 = d1.count();

    const double mcups1 = (total_cells / seconds1) / 1e6; // Mega Cell Updates / sec
    const double gcups1 = mcups1 / 1000.0;               // Giga Cell Updates / sec

    // 4. Salida por consola (fuera del bloque de tiempo)
    std::cout << "--- STRIP SMITH-WATERMAN RISC-V RVV (LMUL=1) ---\n"
          << "Alignment: I=" << std::get<0>(r1) 
          << " J=" << std::get<1>(r1) 
          << " SCORE=" << std::get<2>(r1) << "\n"
          << "Execution Time: " << seconds1 << " s (" << seconds1 * 1000.0 << " ms)\n"
          << "Throughput:     " << mcups1 << " MCUPS (" << gcups1 << " GCUPS)\n" 
          << std::endl;

    
    ssw::Workspace ws_test = ssw::Workspace(query.size()+1,ssw::vScore(database.size()+1,0));
    
    query.insert(query.begin(),ssw::Base{35}); //not used 
    database.insert(database.begin(),ssw::Base{35}); //not used
    
    const auto s2 = std::chrono::steady_clock::now();
    Result r2 = ssw::smith_waterman(query, database, ws_test);
    const auto e2 = std::chrono::steady_clock::now();

    // barrier avoiding compilers reordenation  
    #if defined(__GNUC__) || defined(__CLANG__)
    asm volatile("" : : "r"(&r2) : "memory");
    #endif


    const std::chrono::duration<double> d2 = e2 - s2;
    const double seconds2 = d2.count();

    const double mcups2 = (total_cells / seconds2) / 1e6; // Mega Cell Updates / sec
    const double gcups2 = mcups2 / 1000.0;               // Giga Cell Updates / sec

    // 4. Salida por consola (fuera del bloque de tiempo)
    std::cout << "--- CLASSIC SMITH-WATERMAN ---\n"
          << "Alignment: I=" << std::get<0>(r2) 
          << " J=" << std::get<1>(r2) 
          << " SCORE=" << std::get<2>(r2) << "\n"
          << "Execution Time: " << seconds2 << " s (" << seconds2 * 1000.0 << " ms)\n"
          << "Throughput:     " << mcups2 << " MCUPS (" << gcups2 << " GCUPS)\n" 
          << std::endl;
    
    auto speedup = seconds2 / seconds1;
    std::cout << "Speedup: " << speedup << std::endl;


    return EXIT_SUCCESS;

}