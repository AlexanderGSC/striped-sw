#include <ssw/rvv-ssw.hpp>
#include <ssw/common.hpp>
#include <ssw/ssw.hpp>
#include <vector>
#include <tuple>
#include <string>
#include <algorithm>



// riscv64-linux-gnu-g++ -march=rv64gcv -mabi=lp64d -std=c++23 -Iinclude -static tests/test.cpp -o testv

int main(int arcg, char** argv) {

    constexpr size_t LMUL = 1;

    Sequence database = ssw::generate_random_sequence(500,0);
    Sequence query    = ssw::generate_random_sequence(500,0);

    std::cout << "L=" << std::setw(4) << query.size() << " QUERY   : "; ssw::print_seq(query, ' ');
    std::cout << "L=" << std::setw(4) << database.size() << " DATABASE: "; ssw::print_seq(database,' ');
    bool correct = true;

    Result r1 = riscv_ssw::sswlu<LMUL>(query, database);
    ssw::Workspace ws_test = ssw::Workspace(query.size()+1,ssw::vScore(database.size()+1,0));
    query.insert(query.begin(),ssw::Base{35}); //not used 
    database.insert(database.begin(),ssw::Base{35}); //not used
    Result r2 = ssw::smith_waterman(query, database, ws_test);
    //ssw::print_workspace(ws_test);
    std::cout << "STRIP SMITH WATERMAN " << std::endl;
    std::cout << "I=" << std::get<0>(r1) << " J=" << std::get<1>(r1) << " SCORE=" << std::get<2>(r1) << std::endl;
    std::cout << "SMITH WATERMAN " << std::endl;
    std::cout << "I=" << std::get<0>(r2) << " J=" << std::get<1>(r2) << " SCORE=" << std::get<2>(r2) << std::endl;
    ssw::Sequence align_db, align_query;
    align_db.reserve(database.size());
    align_query.reserve(query.size());
    ssw::backtrack(ws_test,database,query,align_db,align_query);
    std::cout << "=========================================\nALIGNMENT\n";
    /*
    for (size_t i=0; i<align_db.size(); ++i) std::cout << ssw::to_char(align_db[i]) << " ";
    std::cout << std::endl;
    for (size_t i=0; i<align_db.size(); ++i) {
        if (align_db[i] == align_query[i] && align_db[i] != '-') std::cout << "| ";
        else std::cout << "  ";
    }
    std::cout << std::endl;
    for (size_t i=0; i<align_db.size(); ++i) std::cout << ssw::to_char(align_query[i]) << " ";
    std::cout << std::endl;
    */
    if (std::get<2>(r1) != std::get<2>(r2)) {
        std::cerr << "Error on alignment." << std::endl;
    }

    if (correct) return EXIT_SUCCESS;
    else return EXIT_FAILURE;
}