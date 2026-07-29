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
    constexpr std::array<size_t,ntests> query_length {1238, 2130, 4130, 270, 101,  53,  91,  900, 456, 390};
    constexpr std::array<size_t,ntests> db_length    {543,   592, 1035, 405, 901, 190, 231, 2091, 890, 790};

    bool correct = true;

    for (size_t i=0; i < ntests; ++i) {

        size_t ql = query_length[i]; 
        size_t dl = db_length[i];

        Sequence database = ssw::generate_random_sequence(ql,0);
        Sequence query    = ssw::generate_random_sequence(dl,0);

        Result r1 = riscv_ssw::strip_smith_waterman<1>(query,database);
        Score  s1 = std::get<2>(r1);
        Result r2 = riscv_ssw::strip_smith_waterman<2>(query,database);
        Score  s2 = std::get<2>(r1);
        Result r4 = riscv_ssw::strip_smith_waterman<4>(query,database);
        Score  s4 = std::get<2>(r1);
        Result r8 = riscv_ssw::strip_smith_waterman<8>(query,database);
        Score  s8 = std::get<2>(r1);

        ssw::Workspace ws_test = ssw::Workspace(query.size()+1,ssw::vScore(database.size()+1,0));
        query.insert(query.begin(),ssw::Base{35}); //not used 
        database.insert(database.begin(),ssw::Base{35}); //not used
        Result r = ssw::smith_waterman(query, database, ws_test);
        Score  s = std::get<2>(r);
        if (s != s1 || s != s2 || s != s4 || s != s8) {
            correct = false;
            break;
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