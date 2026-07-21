#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <ranges>

#include <riscv_vector.h>
#include "rvv-traits.hpp"
#include "common.hpp"

using namespace ssw;

namespace riscv_ssw {

constexpr size_t LMUL = 1;
constexpr size_t SEW  = sizeof(Score);

Workspace generate_query_profile(Sequence& q) { 
    const size_t VLEN= __riscv_vlenb(); // bytes
    const size_t sizeReg = (VLEN * LMUL) / (SEW); // 32*1/2 = 16 elems 
    size_t numRegs = (q.size() + sizeReg - 1) / sizeReg; // 38/16 = 3 registros
    numRegs = (numRegs < 2) ? 2 : numRegs; //at least 2 regs
    
    const size_t efectiveVLEN = (q.size()+numRegs-1) / numRegs;

    Workspace qp(num_bases, vScore(numRegs * sizeReg, 0));
    
    for (Base a : all_bases) {
        const size_t max_vl = sizeReg;
        size_t i = 0;
        for (size_t n=0; n<numRegs; ++n ) { 
            vint16m1_t val = __riscv_vmv_v_x_i16m1(0,max_vl); //val=0
            size_t vl = 0;
            for (size_t k=0; k<efectiveVLEN; ++k) 
                if (n + k*numRegs < q.size()) vl++;
                else break;
            //std::cout << "N= " << n << " EFECTIVE VLEN: " << vl << " MAX VL: " << max_vl << " i=" << i << " REMAINING: " << q.size()-i << std::endl;
            if (vl > 0) {
                vuint8mf2_t idx = __riscv_vlse8_v_u8mf2(&q[n], 
                        static_cast<ptrdiff_t>(numRegs), vl);
                idx = __riscv_vsll_vx_u8mf2(idx, 1, vl);
                // loads Score (int16_t) from score matrix for elem a with stride = idx
                //loading a max of 32 elements requires a LMUL=2 for int16_t
                val = __riscv_vloxei8_v_i16m1(score[a].data(),idx, vl);
            }
            __riscv_vse16_v_i16m1(&qp[a][n*max_vl],
                                   val,
                                   max_vl);
        }
    }
    return qp;
}


Workspace generate_query_profile2(Sequence& q) {
    using Traits = rvv_traits<int16_t,1>;
    using T = Traits::elem_vector_type;
    using I = Traits::index_vector_type;

    const size_t VLEN= __riscv_vlenb(); // en bytes
    const size_t sizeReg = (VLEN * LMUL) / (SEW); // 32*1/2 = 16 elems 
    size_t numRegs = (q.size() + sizeReg - 1) / sizeReg; // 38/16 = 3 registros
    numRegs = (numRegs < 2) ? 2 : numRegs; //at least 2 regs
    
    const size_t efectiveVLEN = (q.size()+numRegs-1) / numRegs;

    Workspace qp(num_bases, vScore(numRegs * sizeReg, 0));
    
    for (Base a : all_bases) {
        const size_t max_vl = sizeReg;
        size_t i = 0;
        for (size_t n=0; n<numRegs; ++n ) { 
            T val = Traits::move(0,max_vl); //val=0
            size_t vl = 0;
            for (size_t k=0; k<efectiveVLEN; ++k) 
                if (n + k*numRegs < q.size()) vl++;
                else break;
            //std::cout << "N= " << n << " EFECTIVE VLEN: " << vl << " MAX VL: " << max_vl << " i=" << i << " REMAINING: " << q.size()-i << std::endl;
            if (vl > 0) {
                I idx = Traits::ldse(&q[n], static_cast<ptrdiff_t>(numRegs), vl);
                idx = Traits::sll(idx, 1, vl);
                // loads Score (int16_t) from score matrix for elem a with stride = idx
                //loading a max of 32 elements requires a LMUL=2 for int16_t
                val = Traits::loxe(score[a].data(),idx, vl);
            }
            Traits::store (&qp[a][n*max_vl], val, max_vl);
        }
    }
    return qp;
}



//  riscv64-linux-gnu-g++ -march=rv64gcv -mabi=lp64d -std=c++23 -static testv.cpp -o testv
Result strip_smith_waterman(Sequence& query, Sequence &database) {
    using Traits = rvv_traits<Score, 1>;
    using T      = Traits::elem_vector_type;
    using I      = Traits::index_vector_type;
    size_t VLEN = __riscv_vlenb();
    const size_t simdLength = (VLEN * LMUL) / SEW;
    size_t niter = (query.size()+simdLength-1) / simdLength;
    niter = (niter < 2) ? 2 : niter;

    const Workspace query_profile = generate_query_profile2(query);
    std::cout << "SIMD REGISTER LENGTH: " << simdLength << " NUMBER OF REGISTERS:" << niter << std::endl;
   
    Workspace vHLoad(niter, vScore(simdLength, 0));
    Workspace vHStore(niter,vScore(simdLength, 0));
    Workspace vE(niter,vScore(simdLength, 0));


    T vF   = Traits::move(Score{0},simdLength);
    T vH   = Traits::move(Score{0},simdLength);
    T vMax = Traits::move(Score{0},simdLength);
    T aux  = Traits::move(Score{0},simdLength);

    size_t it = 1, max_i=0, max_j=0;
    Score max_score = Score{0};
    
    for (Base db : database) {

        vF = Traits::move(0,simdLength); //vF = 0
        aux= Traits::load(vHStore[niter-1].data(), simdLength);
        vH = Traits::slideup_zero(aux,1,simdLength);
        //shiftright_score(vHStore[niter-1], vH); //vH = shift(vHStore[niter-1])
        std::swap(vHLoad, vHStore);
        for (size_t j=0; j < niter; ++j) {
            size_t stride = j * simdLength;
            //std::cout << "-------- IT=" << it << " DB=" << riscv_ssw::to_char(db) << " j=" << j << " stride=" << stride << "--------\n";
            //cargar el query profile con el stride y sumar a vH
            //load(aux, query_profile[db],stride); //aux carga el profile
            //add(vH,aux);       //vH = vH + aux
            aux  = Traits::load((Score *)&query_profile[db][stride], simdLength);
            vH   = Traits::add(vH, aux, simdLength);
            vMax = Traits::max(vH,vMax, simdLength);
            //max(vMax,vH); //vMax = max(vH,vMax)
            // check if vMax has greater value than max_score
            //size_t max_idx = max_index(vMax);
            Score max_v = Traits::max(vMax,simdLength);
            if (max_v > max_score) {
                size_t max_idx = (size_t) Traits::max_idx(vMax,max_v,simdLength);
                max_score = max_v;
                max_i = it-1;
                max_j = j + max_idx*niter;
                //std::cout << "New max val found! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            }
            aux = Traits::load(vE[j].data(), simdLength); 
            vH  = Traits::max(vH, aux, simdLength);
            //max(vH,vE[j]);  //vH = max(vH,vE[j])
            vH  = Traits::max(vH, vF, simdLength);
            //max(vH,vF);     //vH = max(vH,vF)
            //std::cout << "vH = "; Traits::print_score(vH);
            
            Traits::store(vHStore[j].data(), vH, simdLength);
            //vHStore[j] = vH;   //vStore[j] = vH

            vH = Traits::add(vH, gap_init, simdLength);
            //add(vH,gap_init);  //vH = vH + gap_init

            aux = Traits::add(aux, gap_extent, simdLength);
            //add(vE[j],gap_extent); // vE[j] = vE[j] + gap_extent

            aux = Traits::max(aux, vH, simdLength);
            Traits::store(vE[j].data(), aux, simdLength);
            //max(vE[j],vH); //vE[j] = max(vH,vE[j])
            vF = Traits::add(vF, gap_extent, simdLength);
            //add(vF,gap_extent);  //vF = vF + gap_extent

            vF = Traits::max(vF, vH, simdLength);
            //max(vF,vH);       //vF = max(vH,vF)
            
            //std::cout << "vE = "; print_score(vE[j]);
            //std::cout << "vM = "; Traits::print_score(vMax,simdLength);
            //std::cout << "vF = "; Traits::print_score(vF,simdLength);
            //std::cout << "aux= "; Traits::print_score(aux,simdLength);

            vH = Traits::load(vHLoad[j].data(), simdLength);
            //vH = vHLoad[j];
        }
        //std::cout << ">> STARTING LAZY F-LOOP FOR DB=" << to_char(db) << " <<\n";
        size_t j = 0;
        vF = Traits::slideup_zero(vF, 1, simdLength);
        //shiftright_score(vF); //shifted_VF = vF << 1


        while (Traits::any_greater(vF, vHStore[j].data(), gap_init, simdLength)) {
            //std::cout << "   [Lazy-F] Corrigiendo bloque j=" << j << " con vF="; //print_score(vF);
            aux = Traits::load(vHStore[j].data(), simdLength);
            aux = Traits::max(aux, vF, simdLength); 
            Traits::store(vHStore[j].data(), aux, simdLength);
            // vHStore[j] = max(vHStore[j],vF)

            vMax= Traits::max(vMax, aux, simdLength);
            //max(vMax,vHStore[j]); //vMax = max(vMax, vHStore[j])

            Score max_v = Traits::max(vMax,simdLength);
            if (max_v > max_score) {
                size_t max_idx = (size_t) Traits::max_idx(vMax,max_v,simdLength);
                max_score = max_v;
                max_i = it-1;
                max_j = j + max_idx*niter;
                //std::cout << "New max val found! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            }
            vF = Traits::add(vF, gap_extent, simdLength);
            //check if vMax has updated
            //size_t max_idx = max_index(vMax);
            //if (vMax[max_idx] > max_score) {
            //    max_score = vMax[max_idx];
            //    max_i = it-1;
            //    max_j = j + max_idx*simdLength;
                //std::cout << "New max val found! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            //} 
            //add(vF,gap_extent); // vF = vF + gap_extent
            //max(vF,vF); //saturates to 0 if any v[i] < 0

            //std::cout << "   [Lazy-F] vHStore[" << j << "] fixed = "; print_score(vHStore[j]);
            if (++j >= niter) {
                vF = Traits::slideup_zero(vF,1,simdLength);
                //shiftright_score(vF);
                j = 0;
            }
        }
        

        //std::cout << " END OF COLUMN DB=" << to_char(db) << "\n\n";
        it++; 
    }

    return std::make_tuple(max_j,max_i,max_score);
    //std::cout << "=== VECT STRIP SMITH WATERMAN ====\n";
    //std::cout << "MAX VAL FOUND " << max_score << std::endl;
    //std::cout << "POSITION AT ROW=" << max_j << " COL=" << max_i << std::endl;
    //std::cout << "=================================\n";
}

}