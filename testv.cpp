#include "rvv-ssw.hpp"
#include "ssw.hpp"
#include <vector>
#include <string>
#include <algorithm>


ssw::Sequence generateSeq(size_t length) {
    ssw::Sequence s(length);
    for (size_t i=0; i<length; ++i) s[i] = i % 4;
    return s;
}

int main(int arcg, char** argv) {

    riscv_ssw::Sequence database, query;

    riscv_ssw::from_string("AGCGTCTCTTAGATAGAGACACAGAGAACAGCATACGAGCAGCATACAACAACACAGACATACTACACTATCATTATTTTCACCAGAACGACAGCATCATACATACATACAGACAGACAGCAGACACTACTACTACTACTACCAACGCAGACGACGACTACATACAGACGACACTACATCAGACGACGACAGCATCATCATCAATACTACTACTACTACGACAGCAGACACTACATCTACTCACATCAGAGACAGCACCTCGGAGATAATGAGAAACGACGACAACAGCAATATCATACATATCACGAGCAGCATACTAACTACTACACTACTCAGACGACAGCACTACATCAT",query);
    riscv_ssw::from_string("AAGGCCTCTGCGCTGAAGATAGAGATAACGATTTCACGACGACACTACAGTACGACACTACAGACATCAAGCACATCACACAGAGCATTATACAGACCAGGAGcACACATCCAGAGCACTAACTACGGACAGCACATCATAACTTACTACAGCAGCAGACCATCATCGACGACGACACTCATCAGAGCAGACGACGACATCCATCAACGACGACCATACATCAGACAGGGATATAACCAGAGTACATACTACTACAGACGACGACAGCATACTATACTACCGACACTACATCATACGAGCAGACAGCATCATTTATATAACCAGGATATATCACCGACACTACATACTACAGACGACAGCAGCATACGAGACGAC",database);
    std::cout << "L=" << std::setw(4) << query.size() << " QUERY   : "; riscv_ssw::print_seq(query, ' ');
    std::cout << "L=" << std::setw(4) << database.size() << " DATABASE: "; riscv_ssw::print_seq(database,' ');

    riscv_ssw::Workspace ws1 = riscv_ssw::generate_query_profile2(query);
    //riscv_ssw::print_workspace(ws);
    
    ssw::Workspace ws2 = ssw::generate_query_profile(query, 16);
    //ssw::print_workspace(ws2);
    bool correct = true;
    if (ws1.size() != ws2.size()) {
        std::cerr << "Error WS DIM 1" << ws1.size() << " does not match " << ws2.size() << std::endl;
        correct = false;
    }
    if (ws1.size() != ws2.size()) {
        std::cerr << "Error WS DIM 2" << ws1[0].size() << " does not match " << ws2[0].size() << std::endl;
        correct = false;
    }
    for (size_t i=0; i<ws1.size() ; ++i) 
        for (size_t j=0; j<ws1[0].size(); ++j) 
            if (ws1[i][j] != ws2[i][j]) {
                std::cerr << "Error: Mismatch at pos " << i << "," << j << std::endl;
                correct = false;
            }
        
    if (correct) std::cout << "QUERY PROFILE GENERATION IS CORRECT" << std::endl;

    riscv_ssw::strip_smith_waterman(query, database);
    ssw::Workspace ws_test = ssw::Workspace(query.size()+1,ssw::vScore(database.size()+1,0));
    query.insert(query.begin(),ssw::Base{35}); //not used 
    database.insert(database.begin(),ssw::Base{35}); //not used
    ssw::smith_waterman(query, database, ws_test);
    //ssw::print_workspace(ws_test);

    ssw::Sequence align_db, align_query;
    align_db.reserve(database.size());
    align_query.reserve(query.size());
    ssw::backtrack(ws_test,database,query,align_db,align_query);
    std::cout << "=========================================\nALIGNMENT\n";
    for (size_t i=0; i<align_db.size(); ++i) std::cout << ssw::to_char(align_db[i]) << " ";
    std::cout << std::endl;
    for (size_t i=0; i<align_db.size(); ++i) {
        if (align_db[i] == align_query[i] && align_db[i] != '-') std::cout << "| ";
        else std::cout << "  ";
    }
    std::cout << std::endl;
    for (size_t i=0; i<align_db.size(); ++i) std::cout << ssw::to_char(align_query[i]) << " ";
    std::cout << std::endl;

    return 0;
}