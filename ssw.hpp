#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <ranges>


namespace ssw {

using Base     = uint8_t;
using Sequence = std::vector<Base>;
using Score    = int16_t;
using vScore   = std::vector<Score>;
using Workspace= std::vector<vScore>;




constexpr Score gap_init   = Score{-1};
constexpr Score gap_extent = Score{-1};
constexpr Score success    = Score{2};
constexpr Score miss       = Score{-1};

constexpr size_t num_bases = 4;
constexpr size_t numB = 256;
constexpr Base A = Base{0};
constexpr Base C = Base{1};
constexpr Base G = Base{2};
constexpr Base T = Base{3};


constexpr std::array<Base,num_bases> all_bases {A,C,G,T};
//constexpr std::array<Base,num_bases> base_to_char {'A','C','G','T'};


constexpr std::array<std::array<Score,num_bases>,num_bases> generate_score () 
{
    std::array<std::array<Score,num_bases>,num_bases> w{};
    for (Base b: all_bases) {
        w[b].fill(miss);
        w[b][b] = success;
    }
    return w;
}

constexpr auto score = generate_score();

constexpr std::array<Base,numB> generate_char_to_base() {
    std::array<Base,numB> w;
    w.fill({Base{5}}); //default char
    w['A'] = ssw::A; w['a'] = ssw::A;
    w['C'] = ssw::C; w['c'] = ssw::C;
    w['G'] = ssw::G; w['g'] = ssw::G;
    w['T'] = ssw::T; w['t'] = ssw::T;
    return w;
}

constexpr auto char_to_base = generate_char_to_base();

constexpr std::array<char,numB> generate_base_to_char() {
    std::array<char,numB> w; 
    w.fill('-');//default char
    w[A] = 'A';
    w[C] = 'C';
    w[G] = 'G';
    w[T] = 'T';
    return w;
}

constexpr auto base_to_char = generate_base_to_char();
 
void from_char(const std::vector<char> origin, Sequence& dest){
    dest.resize(origin.size());
    std::transform(origin.begin(), origin.end(), dest.begin(), 
    [](char c){return char_to_base[c];});
}

void from_string(const std::string& s, Sequence& dest) {
    dest.resize(s.size());
    std::transform(s.begin(), s.end(), dest.begin(),
    [=](char c){return char_to_base[c];});
}

char inline to_char(const Base b) {
    return base_to_char[b];
}

void print_seq(const Sequence& seq, const char sep) {
    for (Base s : seq) std::cout << std::setw(1) << to_char(s);
    std::cout << std::endl;
}

void print_score(const vScore& vS) {
    for (Score s : vS) std::cout << std::setw(3) << s << " ";
    std::cout << std::endl;
}

void print_workspace(const Workspace& ws) {
    for (vScore vs: ws) print_score(vs);
}

void inline shiftright_seq(Sequence& seq) {
    std::rotate(seq.begin(), seq.end()-1, seq.end());
}

void inline shiftright_score(const vScore& vs, vScore& dest) {
    std::copy(vs.begin(), vs.end(), dest.begin());
    std::rotate(dest.begin(), dest.end()-1, dest.end());
    dest[0] = Score{0};
}

void inline shiftright_score(vScore& v) {
    std::rotate(v.begin(), v.end()-1, v.end());
    v[0] = Score{0};
}

void inline copy_seq(const Sequence& src, Sequence& dst ) {
    dst.resize(src.size());
    std::copy(src.begin(), src.end(), dst.begin());
}

void eval(const Sequence& src1, const Sequence& src2, 
          vScore& vEval, size_t idx) {
    for (auto i=idx; i<src1.size(); ++i) vEval[i] = score[src1[i]][src2[i]];
}

void eval(const Sequence& src, const Base b,
        vScore& vEval, const size_t idx) {
    for (auto i=idx; i<src.size(); ++i) 
                vEval[i] = score[b][src[i]];       
}

void print_workspace(const Sequence& db, const Sequence& q,
    const Workspace& ws) {
    const size_t ns = 3;
    std::cout << std::setw(ns) << "   ";
    for (size_t i=0; i<db.size(); ++i) std::cout << std::setw(ns) << to_char(db[i]);
    std::cout << std::endl;
    for (size_t i=0; i<ws.size(); ++i) {
        std::cout << std::setw(ns) << to_char(q[i]);
        for (size_t j=0; j<ws[i].size();++j) 
            std::cout << std::setw(ns) << ws[i][j];
        std::cout << std::endl;
    }
}

void inline max(vScore& v1, const vScore& v2) {
    std::ranges::transform(v1, v2, v1.begin(), 
    [](Score a, Score b){return std::max({a,b, Score{0}}); });
}

void inline add(vScore& v, const Score s) {
    std::ranges::transform(v, v.begin(),
        [s](Score a){return a+s; });
}

void inline add(vScore& v, const vScore& qp) {
    std::ranges::transform(v, qp, v.begin(),
       [](Score a, Score b){return a+b; });
}

void inline load(vScore& dest, const vScore& src, const size_t stride) 
{
    size_t end = stride + dest.size();
    std::copy(src.begin()+stride, src.begin()+end, dest.begin());
}

size_t inline max_index(vScore& v) {
    Score max {v[0]};
    size_t idx = 0;
    for (size_t i=1; i<v.size(); ++i) 
        if (v[i] > max) {
            max = v[i];
            idx = i; 
        }
    return idx;    
}

bool inline any_greater(vScore& v1, vScore& v2, Score gap) {
    // return false if any of v1[i] vals is greater than v2[i]
    // for any i
    bool any_greater = false;
    for (size_t i=0; i<v1.size() && i<v2.size(); ++i) 
        any_greater = any_greater || (v1[i] > v2[i] + gap);
    return any_greater;
}

void split_sequence(const Sequence& src, Sequence& s1, Sequence& s2) {
    s1.clear();
    s2.clear();
    s1.reserve((src.size() + 1) / 2);
    s2.reserve(src.size() / 2);
    
    for (size_t i=0; i<src.size(); i+=2) {
        s1.push_back(src[i]);
        if (i+1 < src.size()) s2.push_back(src[i+1]);
    }
}

Workspace generate_query_profile(Sequence& q, size_t sizeReg) { 
    size_t numRegs = (q.size() + sizeReg - 1) / sizeReg;
    numRegs = (numRegs < 2) ? size_t{2} : numRegs;
    Workspace qp(num_bases, 
                vScore(numRegs * sizeReg, 0));
    for (Base a : all_bases) {
        size_t h = 0;
        for (size_t i=0; i<numRegs; ++i) {
            size_t j = i;
            for (size_t k=0; k<sizeReg; ++k) {
                if (j < q.size()) qp[a][h] = score[a][q[j]];
                h++;
                j+=numRegs;
            }
        }
    }
    return qp;
} 

void strip_smith_waterman(Sequence& query, Sequence &database) {
    const size_t simdLength   = 16; //16 elements per vector
    const size_t niter = (query.size()+simdLength-1) / simdLength;
    //std::cout << "QUERY PROFILE" << std::endl;
    const Workspace query_profile = generate_query_profile(query, simdLength);
    //print_workspace(query_profile);
    //std::cout << "SIMD REGISTER LENGTH: " << simdLength << " NUMBER OF REGISTERS:" << niter << std::endl;
    
    Workspace vHLoad(niter, vScore(simdLength, 0));
    Workspace vHStore(niter,vScore(simdLength, 0));
    Workspace vE(niter,vScore(simdLength, 0));
    vScore vF(simdLength, 0);
    vScore vH(simdLength, 0);
    vScore vMax(simdLength, 0);
    vScore aux(simdLength,0); 
    size_t it = 1, max_i=0, max_j=0;
    Score max_score = Score{0};
    for (Base db : database) {
        std::fill(vF.begin(),vF.end(),Score{0});
        shiftright_score(vHStore[niter-1], vH); //vH = shift(vHStore[niter-1])
        std::swap(vHLoad, vHStore);
        for (size_t j=0; j < niter; ++j) {
            size_t stride = j * simdLength;
            //std::cout << "-------- IT=" << it << " DB=" << ssw::to_char(db) << " j=" << j << " stride=" << stride << "--------\n";
            //cargar el query profile con el stride y sumar a vH
            load(aux, query_profile[db],stride); //aux carga el profile
            add(vH,aux);       //vH = vH + aux
            max(vMax,vH); //vMax = max(vH,vMax)
            // check if vMax has greater value than max_score
            size_t max_idx = max_index(vMax);
            if (vMax[max_idx] > max_score) {
                max_score = vMax[max_idx];
                max_i = it-1;
                max_j = j + max_idx*niter;
                //std::cout << "New max val found! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            } 
            max(vH,vE[j]);  //vH = max(vH,vE[j])
            max(vH,vF);     //vH = max(vH,vF)
            //std::cout << "vH = "; print_score(vH);
            vHStore[j] = vH;   //vStore[j] = vH
            add(vH,gap_init);  //vH = vH + gap_init
            add(vE[j],gap_extent); // vE[j] = vE[j] + gap_extent
            max(vE[j],vH); //vE[j] = max(vH,vE)
            add(vF,gap_extent);  //vF = vF + gap_extent
            max(vF,vH);       //vF = max(vH,vF)
            
            //std::cout << "vE = "; print_score(vE[j]);
            //std::cout << "vM = "; print_score(vMax);
            //std::cout << "vF = "; print_score(vF);
            //std::cout << "aux= "; print_score(aux);
            vH = vHLoad[j];
        }
        //std::cout << ">> STARTING LAZY F-LOOP FOR DB=" << to_char(db) << " <<\n";
        size_t j = 0;
        shiftright_score(vF); //shifted_VF = vF << 1
        while (any_greater(vF, vHStore[j], gap_init)) {
            //std::cout << "   [Lazy-F] Corrigiendo bloque j=" << j << " con vF="; print_score(vF);
            max(vHStore[j],vF); // vHStore[j] = max(vHStore[j],vF)
            max(vMax,vHStore[j]); //vMax = max(vMax, vHStore[j])
            //check if vMax has updated
            size_t max_idx = max_index(vMax);
            if (vMax[max_idx] > max_score) {
                max_score = vMax[max_idx];
                max_i = it-1;
                max_j = j + max_idx*simdLength;
                //std::cout << "New max val found! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            } 
            add(vF,gap_extent); // vF = vF + gap_extent
            //max(vF,vF); //saturates to 0 if any v[i] < 0

            //std::cout << "   [Lazy-F] vHStore[" << j << "] fixed = "; print_score(vHStore[j]);
            if (++j >= niter) {
                shiftright_score(vF);
                j = 0;
            }
        }
        

        //std::cout << " END OF COLUMN DB=" << to_char(db) << "\n\n";
        it++; 
    }

    std::cout << "===== STRIP SMITH WATERMAN ======\n";
    std::cout << "MAX VAL FOUND " << max_score << std::endl;
    std::cout << "POSITION AT ROW=" << max_j << " COL=" << max_i << std::endl;
    std::cout << "=================================\n";
}


void smith_waterman(const Sequence& query, const Sequence& database,
        Workspace& ws)
{
    Score max_score = Score{0};
    size_t max_i=1, max_j=1;
    for (auto i=1; i<query.size(); ++i) {
        for (auto j=1; j<database.size(); ++j) {
            Base rowv = query[i]; 
            Base colv = database[j];
            ws[i][j] = std::max<int>({ws[i-1][j-1]+score[rowv][colv], 
            ws[i-1][j] + gap_init, ws[i][j-1] + gap_init, 0});
            if (ws[i][j] > max_score) {
                max_score = ws[i][j];
                max_i = i; max_j=j;
            }
        }
    }
    std::cout << "======== SMITH WATERMAN =========\n";
    std::cout << "MAX VAL FOUND " << max_score << std::endl;
    std::cout << "POSITION AT ROW=" << max_i-1 << " COL=" << max_j-1 << std::endl;
    std::cout << "=================================\n";
}


void backtrack(const Workspace& ws, 
    const Sequence& database, const Sequence& query,
    Sequence& align_database, Sequence& align_query) {

    size_t max_i=0, max_j=0;
    for (size_t i=0; i < query.size() ; ++i) {
        for (size_t j=0; j < database.size(); ++j) {
            if (ws[i][j] > ws[max_i][max_j]) {
                max_i=i; max_j=j;
            }
        }
    }

    //std::cout << "Max row=" << max_i << " col=" << max_j << " v= " << ws[max_i][max_j] << std::endl; 

    size_t i = max_i, j = max_j;
    Score v = ws[i][j];
    while (v > 0 && i > 0 && j > 0)
    {
        //std::cout << "i=" << i << " j=" << j << " q[i]=" << static_cast<uint32_t>(query[i]) 
        //    << " d[j]=" << static_cast<uint32_t>(database[j])
        //    << " w[i][j]=" << ws[i][j];
        if (ws[i-1][j-1]+score[query[i]][database[j]] == v)
        {
            //std::cout << "<-- main diag" << std::endl;
            if (query[i] == database[j]) {
                align_query.push_back(query[i]);
                align_database.push_back(database[j]); 
            } else {
                align_query.push_back('-');
                align_database.push_back('-');
            }
            --i; --j;
        } else if (ws[i][j-1]+gap_init == v) {
            //std::cout << "<-- query stride" << std::endl;
            align_query.push_back('E');
            align_database.push_back(database[j]);
            --j;
        } else if (ws[i-1][j]+gap_init == v) {
            //std::cout << "<-- database stride" << std::endl;
            align_query.push_back(query[i]);
            align_database.push_back('E');
            --i;
        }
        v = ws[i][j];
    }
    std::reverse(align_query.begin(),align_query.end());
    std::reverse(align_database.begin(),align_database.end());
}



}