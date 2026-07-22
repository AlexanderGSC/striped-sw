#include <ssw/rvv-ssw.hpp>
#include <ssw/common.hpp>
#include <ssw/ssw.hpp>
#include <vector>
#include <tuple>
#include <string>
#include <algorithm>


// riscv64-linux-gnu-g++ -march=rv64gcv -mabi=lp64d -std=c++23 -Iinclude -static tests/test.cpp -o testv

int main(int arcg, char** argv) {

    constexpr size_t ntests = 10;
    constexpr std::array<size_t,ntests> query_length {123, 213, 413, 327, 101, 53, 91, 900, 456, 390};
    constexpr std::array<size_t,ntests> db_length    {543, 592, 1035,405, 901, 190, 231, 2091, 890, 790};
    constexpr std::array<size_t,ntests> q_seeds {73, 39845, 389293, 29834, 832901, 92373, 26271, 23892, 38471, 91383};
    constexpr std::array<size_t,ntests> d_seeds {728, 47822, 3737, 3828, 23893, 2912, 3928, 17364, 382919, 8281};

    bool correct = true;

    for (size_t i=0; i < ntests; ++i) {
        size_t ql = query_length[i]; 
        size_t dl = db_length[i];
        size_t q_seed = q_seeds[i];
        size_t d_seed = d_seeds[i];

        Sequence database = ssw::generate_random_sequence(ql,q_seed);
        Sequence query    = ssw::generate_random_sequence(dl,d_seed);


        Result r1 = riscv_ssw::strip_smith_waterman(query, database);

        ssw::Workspace ws_test = ssw::Workspace(query.size()+1,ssw::vScore(database.size()+1,0));
        query.insert(query.begin(),ssw::Base{35}); //not used 
        database.insert(database.begin(),ssw::Base{35}); //not used
        Result r2 = ssw::smith_waterman(query, database, ws_test);
    
        std::cout << "STRIP SMITH WATERMAN " << std::endl;
        std::cout << "I=" << std::get<0>(r1) << " J=" << std::get<1>(r1) << " SCORE=" << std::get<2>(r1) << std::endl;
        std::cout << "SMITH WATERMAN " << std::endl;
        std::cout << "I=" << std::get<0>(r2) << " J=" << std::get<1>(r2) << " SCORE=" << std::get<2>(r2) << std::endl;

        if (std::get<2>(r1) != std::get<2>(r2)) {
            correct = false;
            std::cerr << "Error on alignment." << std::endl;
        }
    }
        if (correct) {
            std::cout << "Test OK ✅" << std::endl;
            return EXIT_SUCCESS;
        }
        else {
            std::cout << "Test Failure ❌" << std::endl;
            return EXIT_FAILURE;
        }
}