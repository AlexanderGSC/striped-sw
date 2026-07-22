#pragma once

#include <riscv_vector.h> 
#include <concepts>

template <typename T, size_t LMUL> struct rvv_traits;

// template specialization for vintm16_t 
template <> struct rvv_traits<int16_t, 1> {
    using elem_vector_type = vint16m1_t;    
    using elem_type   = int16_t;    
    using index_vector_type  = vuint8mf2_t;
    using index_type = uint8_t;

    static inline size_t setvl(size_t n) 
        {return __riscv_vsetvl_e16m1(n);}
    
    static inline vint16m1_t load(int16_t* p, size_t vl) 
        {return __riscv_vle16_v_i16m1(p,vl);}
    
    static inline vint16m1_t move(int16_t v, size_t vl)  
        {return __riscv_vmv_v_x_i16m1(v,vl);}

    static inline vint16m1_t slideup_zero(vint16m1_t v, size_t offset, size_t vl) {
        vint16m1_t zeros = __riscv_vmv_v_x_i16m1(0,vl);
        return __riscv_vslideup_vx_i16m1(zeros, v, offset, vl);}

    static inline vint16m1_t loxe(const elem_type* p, index_vector_type i, size_t vl)
        {return __riscv_vloxei8_v_i16m1(p, i, vl);}

    static inline vuint8mf2_t ldse(const uint8_t* p, size_t stride, size_t vl) 
        {return __riscv_vlse8_v_u8mf2(p,static_cast<ptrdiff_t>(stride),vl);}

    static inline vint16m1_t add(vint16m1_t v1,vint16m1_t v2, size_t vl) 
        {return __riscv_vadd_vv_i16m1(v1,v2,vl);}

    static inline vint16m1_t add(vint16m1_t v, int16_t i, size_t vl) 
        {return __riscv_vadd_vx_i16m1(v,i,vl);}

    static inline vint16m1_t addsat(vint16m1_t v1, vint16m1_t v2, size_t vl)
        {   vint16m1_t sum = __riscv_vsadd_vv_i16m1(v1,v2,vl);
            return __riscv_vmax_vx_i16m1(sum,0,vl);}

    static inline vint16m1_t max(vint16m1_t v1, vint16m1_t v2, size_t vl)
        {   v1 = __riscv_vmax_vv_i16m1(v1,v2,vl);
            return __riscv_vmax_vx_i16m1(v1,0,vl);}

    static inline long max_idx(vint16m1_t v1, int16_t val, size_t vl) {
        vbool16_t max_mask = __riscv_vmseq_vx_i16m1_b16(v1,val,vl);
        return __riscv_vfirst_m_b16(max_mask, vl);}

    static inline int16_t max(vint16m1_t v, size_t vl) {
            vint16m1_t red_dest = __riscv_vmv_v_x_i16m1(INT16_MIN,1);
            red_dest = __riscv_vredmax_vs_i16m1_i16m1(v,red_dest,vl);
            return __riscv_vmv_x_s_i16m1_i16(red_dest);}

    static inline vint16m1_t sll(vint16m1_t v, size_t i, size_t vl) 
        {return __riscv_vsll_vx_i16m1(v,i,vl);}

    static inline vuint8mf2_t sll(vuint8mf2_t v, size_t i, size_t vl)
        {return __riscv_vsll_vx_u8mf2(v,i,vl);}

    static inline void store(int16_t* p, vint16m1_t v, size_t vl) 
        {return __riscv_vse16_v_i16m1(p,v,vl);}

    static inline bool any_greater(vint16m1_t v1, int16_t* p, int16_t g, size_t vl) 
    {
        vint16m1_t v2  = __riscv_vle16_v_i16m1(p, vl);
        v2             = __riscv_vadd_vx_i16m1(v2, g, vl);
        vbool16_t mask = __riscv_vmsgt_vv_i16m1_b16(v1, v2, vl);
        long first_idx = __riscv_vfirst_m_b16(mask,vl);
        return (first_idx != -1);      
    }
    
    static inline void print_score(vint16m1_t v, size_t vl) {
        std::vector<int16_t> aux(vl,0);
        __riscv_vse16_v_i16m1(aux.data(),v,vl);
        for (int16_t s : aux) std::cout << s << " ";
        std::cout << std::endl;
    }
};

template <> struct rvv_traits<int16_t, 2> {};
template <> struct rvv_traits<int16_t, 4> {};
template <> struct rvv_traits<int16_t, 8> {};


template <> struct rvv_traits<int32_t, 1> {
    static inline size_t setvl(size_t n) {return __riscv_vsetvl_e32m1(n);}
    static inline vint32m1_t load(int32_t* p, size_t n) {return __riscv_vle32_v_i32m1(p,n);}
    static inline vint32m1_t move(int32_t  v, size_t n) {return __riscv_vmv_v_x_i32m1(v,n);}
    static inline void store(int32_t* p, vint32m1_t v, size_t vl) {return __riscv_vse32_v_i32m1(p,v,vl);}
};

template <> struct rvv_traits<int32_t, 2> {};
template <> struct rvv_traits<int32_t, 4> {};
template <> struct rvv_traits<int32_t, 8> {};


template <> struct rvv_traits<int64_t, 1> {};
template <> struct rvv_traits<int64_t, 2> {};
template <> struct rvv_traits<int64_t, 4> {};
template <> struct rvv_traits<int64_t, 8> {};