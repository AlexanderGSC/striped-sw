#include "ssw.hpp"
#include <vector>
#include <string>
#include <algorithm>


int main(int arcg, char** argv) {

    ssw::Sequence database, query;
    ssw::from_string("AGTCGATAGCTGACATCGATACGAGCTGGCTAGGACATTCACGATACGAGATCACATACAGACATGCACCACGGGACATTTCAAGTAGTAGACAGTAGCTAACCTAGGATC",database);
    ssw::from_string("AGCTTGCGAGTACAGGCTACGATTACAAGT", query);

    std::cout << "L=" << std::setw(4) << query.size() << " QUERY   : "; ssw::print_seq(query, ' ');
    std::cout << "L=" << std::setw(4) << database.size() << " DATABASE: "; ssw::print_seq(database,' ');

    ssw::strip_smith_waterman(query, database);
    ssw::Workspace ws(query.size()+1,ssw::vScore(database.size()+1,0));
    query.insert(query.begin(),ssw::Base{35}); //not used 
    database.insert(database.begin(),ssw::Base{35}); //not used
    ssw::smith_waterman(query,database,ws);
    //ssw::print_workspace(database, query, ws);
    std::cout << "=========================================\nBACKTRACK\n";
    ssw::Sequence align_db, align_query;
    align_db.reserve(database.size());
    align_query.reserve(query.size());
    ssw::backtrack(ws,database,query,align_db,align_query);
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