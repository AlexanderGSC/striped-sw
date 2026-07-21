#pragma once

#include "common.hpp"

#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <ranges>

using namespace ssw;

namespace ssw {

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





}