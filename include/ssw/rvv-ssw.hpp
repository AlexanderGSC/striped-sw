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

constexpr size_t SEW  = sizeof(Score);

Workspace generate_query_profile(Sequence& q) { 
    const size_t LMUL= 1;
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

template <std::size_t LMUL>
Workspace generate_query_profile2(const Sequence& q) {
    //const size_t LMUL = 2;
    using Traits = rvv_traits<int16_t,LMUL>;
    using T = Traits::vector_t;
    using I = Traits::index_v_t;

    const size_t VLEN= __riscv_vlenb(); // bytes
    //const size_t sizeReg = (VLEN * LMUL) / (SEW);  
    const size_t sizeReg = Traits::setvl(-1);
    size_t numRegs = (q.size() + sizeReg - 1) / sizeReg; 
    numRegs = (numRegs < 2) ? 2 : numRegs; //at least 2 regs
    
    const size_t efectiveVLEN = (q.size()+numRegs-1) / numRegs;

    Workspace qp(num_bases, vScore(numRegs * sizeReg, 0));
    
    for (Base a : all_bases) {
        //const size_t max_vl = sizeReg;
        const size_t max_vl = Traits::setvl(-1);
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
                // loads Score from score matrix for elem a with stride = idx
                //loading a max of 32 elements requires a LMUL=2 for int16_t
                val = Traits::loxe(score[a].data(),idx, vl);  
            }
            Traits::store(&qp[a][n*max_vl], val, vl);
        }
    }
    return qp;
}


template <std::size_t LMUL>
Workspace gqp(const Sequence& q, const size_t numRegs) {
    //const size_t LMUL = 2;
    using Traits = rvv_traits<int16_t,LMUL>;
    using T = Traits::vector_t;
    using I = Traits::index_v_t;
  
    const size_t sizeReg = Traits::setvl(-1);
    const size_t efectiveVLEN = (q.size()+numRegs-1) / numRegs;

    Workspace qp(num_bases, vScore(numRegs * sizeReg, 0));
    
    for (Base a : all_bases) {
        //const size_t max_vl = sizeReg;
        const size_t max_vl = Traits::setvl(-1);
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
                // loads Score from score matrix for elem a with stride = idx
                //loading a max of 32 elements requires a LMUL=2 for int16_t
                val = Traits::loxe(score[a].data(),idx, vl);  
            }
            Traits::store(&qp[a][n*max_vl], val, vl);
        }
    }
    return qp;
}



//  riscv64-linux-gnu-g++ -march=rv64gcv -mabi=lp64d -std=c++23 -static testv.cpp -o testv
template <std::size_t LMUL>
Result strip_smith_waterman(Sequence& query, Sequence &database) {
    //const size_t LMUL = 2;
    using Traits = rvv_traits<Score, LMUL>;
    using T      = Traits::vector_t;
    using I      = Traits::index_v_t;
    size_t VLEN = __riscv_vlenb();
    const size_t simdLength = Traits::setvl(-1);
    size_t niter = (query.size()+simdLength-1) / simdLength;
    niter = (niter < 2) ? 2 : niter;

    const Workspace query_profile = generate_query_profile2<LMUL>(query);
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
                std::cout << "New max val found! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
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



template <std::size_t LMUL>
Result sswlu(Sequence& query, Sequence &database) {

    using Traits = rvv_traits<Score, LMUL>;
    using T      = Traits::vector_t;
    using I      = Traits::index_v_t;

    const size_t simdLength = Traits::setvl(-1);
    size_t niter = (query.size()+simdLength-1) / simdLength;
    niter = (niter < 4) ? 4 : niter;
    niter = (niter % 4 == 0) ? niter : ((niter / 4) + 1) * 4;
    const Workspace query_profile = gqp<LMUL>(query,niter);

    //std::cout << "SIMD REGISTER LENGTH: " << simdLength << " NUMBER OF REGISTERS:" << niter << std::endl;

   
    Workspace vHLoad(niter, vScore(simdLength, 0));
    Workspace vHStore(niter,vScore(simdLength, 0));
    Workspace vE(niter,vScore(simdLength, 0));

    T vF   = Traits::move(Score{0},simdLength);

    T vH0   = Traits::move(Score{0},simdLength);
    T vH1   = Traits::move(Score{0},simdLength);
    T vH2   = Traits::move(Score{0},simdLength);
    T vH3   = Traits::move(Score{0},simdLength);

    T vMax  = Traits::move(Score{0},simdLength);
    T aux   = Traits::move(Score{0},simdLength);

    size_t it = 1, max_i=0, max_j=0;
    Score max_score = Score{0};

    for (Base db : database) {

        vF = Traits::move(Score{0},simdLength);
        
        std::swap(vHLoad, vHStore);

        aux = Traits::load(vHLoad[niter-1].data(), simdLength);
        vH0 = Traits::slideup_zero(aux,1,simdLength);

        
        for (size_t j=0; j < niter; j += 4) {

            T vE_j0, vE_j1, vE_j2, vE_j3;

            size_t stride0 = (j+0) * simdLength;
            size_t stride1 = (j+1) * simdLength;
            size_t stride2 = (j+2) * simdLength;
            size_t stride3 = (j+3) * simdLength;

            //std::cout << "-------- IT=" << it << " DB=" << ssw::to_char(db) << " j=" << j << " stride=" << stride1 << "--------\n";

            // BLOCK 0
            aux  = Traits::load((Score *)&query_profile[db][stride0], simdLength);
            vH0  = Traits::add(vH0,  aux, simdLength);
            vMax = Traits::max(vH0, vMax, simdLength);
            vE_j0= Traits::load(vE[j+0].data(), simdLength);
            vH0  = Traits::max(vH0, vE_j0, simdLength);
            vH0  = Traits::max(vH0, vF, simdLength);
            Traits::store(vHStore[j+0].data(), vH0, simdLength);
            Score max_v0 = Traits::max(vMax,simdLength);
            if (max_v0 > max_score) {
                size_t max_idx = (size_t) Traits::max_idx(vMax,max_v0,simdLength);
                max_score = max_v0;
                max_i = it-1;
                max_j = (j + 0) + max_idx * niter;
                //std::cout << "New max val found1! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            }

            vH0  = Traits::add(vH0, gap_init, simdLength);
            vE_j0= Traits::add(vE_j0, gap_extent, simdLength);
            vE_j0= Traits::max(vE_j0, vH0, simdLength);
            Traits::store(vE[j+0].data(), vE_j0, simdLength);
            vF   = Traits::add(vF,  gap_extent, simdLength);
            vF   = Traits::max(vF, vH0, simdLength);

            // BLOCK 1
            vH1  = Traits::load(vHLoad[j+0].data(), simdLength);
            aux  = Traits::load((Score *)&query_profile[db][stride1], simdLength);
            vH1  = Traits::add(vH1,  aux, simdLength);
            vMax = Traits::max(vH1, vMax, simdLength);
            vE_j1= Traits::load(vE[j+1].data(), simdLength);
            vH1  = Traits::max(vH1, vE_j1, simdLength);
            vH1  = Traits::max(vH1, vF, simdLength);
            Traits::store(vHStore[j+1].data(), vH1, simdLength);
            Score max_v1 = Traits::max(vMax,simdLength);
            if (max_v1 > max_score) {
                size_t max_idx = (size_t) Traits::max_idx(vMax,max_v1,simdLength);
                max_score = max_v1;
                max_i = it-1;
                max_j = (j + 1) + max_idx * niter;
                //std::cout << "New max val found1! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            }

            vH1  = Traits::add(vH1, gap_init, simdLength);
            vE_j1= Traits::add(vE_j1, gap_extent, simdLength);
            vE_j1= Traits::max(vH1, vE_j1, simdLength);
            Traits::store(vE[j+1].data(), vE_j1, simdLength);
            vF   = Traits::add(vF,  gap_extent, simdLength);
            vF   = Traits::max(vF, vH1, simdLength);

            // BLOCK 2
            vH2  = Traits::load(vHLoad[j+1].data(), simdLength);
            aux  = Traits::load((Score *)&query_profile[db][stride2], simdLength);
            vH2  = Traits::add(vH2,  aux, simdLength);
            vMax = Traits::max(vH2, vMax, simdLength);
            vE_j2= Traits::load(vE[j+2].data(), simdLength);
            vH2  = Traits::max(vH2, vE_j2, simdLength);
            vH2  = Traits::max(vH2, vF, simdLength);
            Traits::store(vHStore[j+2].data(), vH2, simdLength);
            Score max_v2 = Traits::max(vMax,simdLength);
            if (max_v2 > max_score) {
                size_t max_idx = (size_t) Traits::max_idx(vMax,max_v2,simdLength);
                max_score = max_v2;
                max_i = it-1;
                max_j = (j + 2) + max_idx * niter;
                //std::cout << "New max val found1! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            }

            vH2  = Traits::add(vH2, gap_init, simdLength);
            vE_j2= Traits::add(vE_j2, gap_extent, simdLength);
            vE_j2= Traits::max(vH2, vE_j2, simdLength);
            Traits::store(vE[j+2].data(), vE_j2, simdLength);
            vF   = Traits::add(vF,  gap_extent, simdLength);
            vF   = Traits::max(vF,  vH2, simdLength);

            // BLOCK 3
            vH3  = Traits::load(vHLoad[j+2].data(), simdLength);
            aux  = Traits::load((Score *)&query_profile[db][stride3], simdLength);
            vH3  = Traits::add(vH3,  aux, simdLength);
            vMax = Traits::max(vH3, vMax, simdLength);
            vE_j3= Traits::load(vE[j+3].data(), simdLength);
            vH3  = Traits::max(vH3, vE_j3, simdLength);
            vH3  = Traits::max(vH3, vF, simdLength);
            Traits::store(vHStore[j+3].data(), vH3, simdLength);
            Score max_v3 = Traits::max(vMax,simdLength);
            if (max_v3 > max_score) {
                size_t max_idx = (size_t) Traits::max_idx(vMax,max_v3,simdLength);
                max_score = max_v3;
                max_i = it-1;
                max_j = (j + 3) + max_idx * niter;
                //std::cout << "New max val found1! v=" << max_score << " i=" << max_i << " j=" << max_j << std::endl;
            }
            vH3  = Traits::add(vH3, gap_init, simdLength);
            vE_j3= Traits::add(vE_j3, gap_extent, simdLength);
            vE_j3= Traits::max(vH3, vE_j3, simdLength);
            Traits::store(vE[j+3].data(), vE_j3, simdLength);
            vF   = Traits::add(vF,  gap_extent, simdLength);
            vF   = Traits::max(vF, vH3, simdLength);

            vH0 = Traits::load(vHLoad[j+3].data(), simdLength);
        }

        // =================================================================
        // CORRECCIÓN DEL LAZY F-LOOP
        // =================================================================
        // 1. Tomamos el vF final generado en el último sub-bloque (vF3)
        // y le inyectamos la diagonal para la primera posición del bloque 0.
        vF = Traits::slideup_zero(vF, 1, simdLength);

        size_t j_lazy = 0;
        bool loop_again = true;

        // Hacemos pasadas hasta que ningún vF supere a vH + gap_init en ningún bloque
        for (size_t r = 0; r < niter && loop_again; ++r) {
            loop_again = false;
            for (size_t k = 0; k < niter; ++k) {
        
                // Cargar el vH actual guardado
                aux = Traits::load(vHStore[k].data(), simdLength);
                // Si algún elemento de vF supera a vH
                if (Traits::any_greater(vF, vHStore[k].data(), gap_init, simdLength)) {
                    loop_again = true;
                    // Actualizar vH con el vF que viene del arrastre
                    aux = Traits::max(aux, vF, simdLength);
                    Traits::store(vHStore[k].data(), aux, simdLength);

                    // Re-evaluar el máximo global si vH aumentó
                    vMax = Traits::max(vMax, aux, simdLength);
                    Score max_v = Traits::max(vMax, simdLength);
                    if (max_v > max_score) {
                        size_t max_idx = (size_t) Traits::max_idx(vMax, max_v, simdLength);
                        max_score = max_v;
                        max_i = it - 1;
                        max_j = max_idx + (k * simdLength);
                    }
                }

                // Propagar vF hacia el siguiente bloque (k + 1):
                // vF_next = max(vF + gap_extent, aux + gap_init)
                T vF_ext  = Traits::add(vF, gap_extent, simdLength);
                T vH_init = Traits::add(aux, gap_init, simdLength);
                vF = Traits::max(vF_ext, vH_init, simdLength);
        
                // Al llegar al último bloque de la fila (k == niter - 1),
                // desplazamos el vector 1 carril para conectar el final con el inicio de la query.
                if (k == niter - 1) {
                    vF = Traits::slideup_zero(vF, 1, simdLength);
                }
            }
        }
        
        it++;
    }
    return std::make_tuple(max_j,max_i,max_score);
}


}