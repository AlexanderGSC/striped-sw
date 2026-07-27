#pragma once

#include <riscv_vector.h> 
#include <concepts>

template <typename T, size_t LMUL> struct rvv_traits;

// template specialization for vintm16_t 
template <> struct rvv_traits<int16_t, 1> {
    using vector_t  = vint16m1_t;    
    using elem_t    = int16_t;    
    using index_v_t = vuint8mf2_t;
    using index_t   = uint8_t;

    static inline size_t setvl(size_t n) 
        {return __riscv_vsetvl_e16m1(n);}
    
    static inline vector_t load(elem_t* p, size_t vl) 
        {return __riscv_vle16_v_i16m1(p,vl);}
    
    static inline vector_t move(elem_t v, size_t vl)  
        {return __riscv_vmv_v_x_i16m1(v,vl);}

    static inline vector_t slideup_zero(vector_t v, size_t offset, size_t vl) {
        vector_t zeros = __riscv_vmv_v_x_i16m1(0,vl);
        return __riscv_vslideup_vx_i16m1(zeros, v, offset, vl);}

    static inline vector_t loxe(const elem_t* p, index_v_t i, size_t vl)
        {return __riscv_vloxei8_v_i16m1(p, i, vl);}

    static inline index_v_t ldse(const index_t* p, size_t stride, size_t vl) 
        {return __riscv_vlse8_v_u8mf2(p,static_cast<ptrdiff_t>(stride),vl);}

    static inline vector_t add(vector_t v1,vector_t v2, size_t vl) 
        {return __riscv_vadd_vv_i16m1(v1,v2,vl);}

    static inline vector_t add(vector_t v, elem_t i, size_t vl) 
        {return __riscv_vadd_vx_i16m1(v,i,vl);}

    static inline vector_t addsat(vector_t v1, vector_t v2, size_t vl)
        {   vint16m1_t sum = __riscv_vsadd_vv_i16m1(v1,v2,vl);
            return __riscv_vmax_vx_i16m1(sum,0,vl);}

    static inline vector_t max(vector_t v1, vector_t v2, size_t vl)
        {   v1 = __riscv_vmax_vv_i16m1(v1,v2,vl);
            return __riscv_vmax_vx_i16m1(v1,0,vl);}

    static inline long max_idx(vector_t v1, elem_t val, size_t vl) {
        vbool16_t max_mask = __riscv_vmseq_vx_i16m1_b16(v1,val,vl);
        return __riscv_vfirst_m_b16(max_mask, vl);}

    static inline elem_t max(vector_t v, size_t vl) {
            vint16m1_t red_dest = __riscv_vmv_v_x_i16m1(INT16_MIN,1);
            red_dest = __riscv_vredmax_vs_i16m1_i16m1(v,red_dest,vl);
            return __riscv_vmv_x_s_i16m1_i16(red_dest);}

    static inline vector_t sll(vector_t v, size_t i, size_t vl) 
        {return __riscv_vsll_vx_i16m1(v,i,vl);}

    static inline index_v_t sll(index_v_t v, size_t i, size_t vl)
        {return __riscv_vsll_vx_u8mf2(v,i,vl);}

    static inline void store(elem_t* p, vector_t v, size_t vl) 
        {return __riscv_vse16_v_i16m1(p,v,vl);}

    static inline bool any_greater(vector_t v1, elem_t* p, int16_t g, size_t vl) 
    {
        vector_t v2    = __riscv_vle16_v_i16m1(p, vl);
        v2             = __riscv_vadd_vx_i16m1(v2, g, vl);
        vbool16_t mask = __riscv_vmsgt_vv_i16m1_b16(v1, v2, vl);
        long first_idx = __riscv_vfirst_m_b16(mask,vl);
        return (first_idx != -1);      
    }
    
    static inline void print_score(vector_t v, size_t vl) {
        std::vector<elem_t> aux(vl,0);
        __riscv_vse16_v_i16m1(aux.data(),v,vl);
        for (elem_t s : aux) std::cout << s << " ";
        std::cout << std::endl;
    }
};


template <> struct rvv_traits<int16_t, 2> 
{
    using vector_t = vint16m2_t;    
    using elem_t   = int16_t;    
    using index_v_t= vuint8m1_t;
    using index_t  = uint8_t;

    static inline size_t setvl(size_t n) 
        {return __riscv_vsetvl_e16m2(n);}
    
    static inline vector_t load(elem_t* p, size_t vl) 
        {return __riscv_vle16_v_i16m2(p,vl);}
    
    static inline vector_t move(elem_t v, size_t vl)  
        {return __riscv_vmv_v_x_i16m2(v,vl);}

    static inline vector_t slideup_zero(vector_t v, size_t offset, size_t vl) {
        vector_t zeros = __riscv_vmv_v_x_i16m2(0,vl);
        return __riscv_vslideup_vx_i16m2(zeros, v, offset, vl);}

    static inline vector_t loxe(const elem_t* p, index_v_t i, size_t vl)
        {return __riscv_vloxei8_v_i16m2(p, i, vl);}

    static inline index_v_t ldse(const index_t* p, size_t stride, size_t vl) 
        {return __riscv_vlse8_v_u8m1(p,static_cast<ptrdiff_t>(stride),vl);}

    static inline vector_t add(vector_t v1, vector_t v2, size_t vl) 
        {return __riscv_vadd_vv_i16m2(v1,v2,vl);}

    static inline vector_t add(vector_t v, elem_t i, size_t vl) 
        {return __riscv_vadd_vx_i16m2(v,i,vl);}

    static inline vector_t addsat(vector_t v1, vector_t v2, size_t vl)
        {   vector_t sum = __riscv_vsadd_vv_i16m2(v1,v2,vl);
            return __riscv_vmax_vx_i16m2(sum,0,vl);}

    static inline vector_t max(vector_t v1, vector_t v2, size_t vl)
        {   v1 = __riscv_vmax_vv_i16m2(v1,v2,vl);
            return __riscv_vmax_vx_i16m2(v1,0,vl);}

    static inline long max_idx(vector_t v1, elem_t val, size_t vl) {
        vbool8_t max_mask = __riscv_vmseq_vx_i16m2_b8(v1,val,vl);
        return __riscv_vfirst_m_b8(max_mask, vl);}

    static inline elem_t max(vector_t v, size_t vl) {
            vint16m1_t red_dest = __riscv_vmv_v_x_i16m1(INT16_MIN,1);
            red_dest = __riscv_vredmax_vs_i16m2_i16m1(v,red_dest,vl);
            return __riscv_vmv_x_s_i16m1_i16(red_dest);}

    static inline vector_t sll(vector_t v, size_t i, size_t vl) 
        {return __riscv_vsll_vx_i16m2(v,i,vl);}

    static inline index_v_t sll(index_v_t v, size_t i, size_t vl)
        {return __riscv_vsll_vx_u8m1(v,i,vl);}

    static inline void store(elem_t* p, vector_t v, size_t vl) 
        {return __riscv_vse16_v_i16m2(p,v,vl);}

    static inline bool any_greater(vector_t v1, elem_t* p, elem_t g, size_t vl) 
    {
        vector_t v2    = __riscv_vle16_v_i16m2(p, vl);
        v2             = __riscv_vadd_vx_i16m2(v2, g, vl);
        vbool8_t mask = __riscv_vmsgt_vv_i16m2_b8(v1, v2, vl);
        long first_idx = __riscv_vfirst_m_b8(mask,vl);
        return (first_idx != -1);      
    }
    
    static inline void print_score(vector_t v, size_t vl) {
        std::vector<elem_t> aux(vl,0);
        __riscv_vse16_v_i16m2(aux.data(),v,vl);
        for (elem_t s : aux) std::cout << s << " ";
        std::cout << std::endl;
    }
};



template <> struct rvv_traits<int16_t, 4> 
{
    using vector_t = vint16m4_t;    
    using elem_t   = int16_t;    
    using index_v_t= vuint8m2_t;
    using index_t  = uint8_t;

    static inline size_t setvl(size_t n) 
        {return __riscv_vsetvl_e16m4(n);}
    
    static inline vector_t load(elem_t* p, size_t vl) 
        {return __riscv_vle16_v_i16m4(p,vl);}
    
    static inline vector_t move(elem_t v, size_t vl)  
        {return __riscv_vmv_v_x_i16m4(v,vl);}

    static inline vector_t slideup_zero(vector_t v, size_t offset, size_t vl) {
        vector_t zeros = __riscv_vmv_v_x_i16m4(0,vl);
        return __riscv_vslideup_vx_i16m4(zeros, v, offset, vl);}

    static inline vector_t loxe(const elem_t* p, index_v_t i, size_t vl)
        {return __riscv_vloxei8_v_i16m4(p, i, vl);}

    static inline index_v_t ldse(const index_t* p, size_t stride, size_t vl) 
        {return __riscv_vlse8_v_u8m2(p,static_cast<ptrdiff_t>(stride),vl);}

    static inline vector_t add(vector_t v1, vector_t v2, size_t vl) 
        {return __riscv_vadd_vv_i16m4(v1,v2,vl);}

    static inline vector_t add(vector_t v, elem_t i, size_t vl) 
        {return __riscv_vadd_vx_i16m4(v,i,vl);}

    static inline vector_t addsat(vector_t v1, vector_t v2, size_t vl)
        {   vector_t sum = __riscv_vsadd_vv_i16m4(v1,v2,vl);
            return __riscv_vmax_vx_i16m4(sum,0,vl);}

    static inline vector_t max(vector_t v1, vector_t v2, size_t vl)
        {   v1 = __riscv_vmax_vv_i16m4(v1,v2,vl);
            return __riscv_vmax_vx_i16m4(v1,0,vl);}

    static inline long max_idx(vector_t v1, elem_t val, size_t vl) {
        vbool4_t max_mask = __riscv_vmseq_vx_i16m4_b4(v1,val,vl);
        return __riscv_vfirst_m_b4(max_mask, vl);}

    static inline elem_t max(vector_t v, size_t vl) {
            vint16m1_t red_dest = __riscv_vmv_v_x_i16m1(INT16_MIN,1);
            red_dest = __riscv_vredmax_vs_i16m4_i16m1(v,red_dest,vl);
            return __riscv_vmv_x_s_i16m1_i16(red_dest);}

    static inline vector_t sll(vector_t v, size_t i, size_t vl) 
        {return __riscv_vsll_vx_i16m4(v,i,vl);}

    static inline index_v_t sll(index_v_t v, size_t i, size_t vl)
        {return __riscv_vsll_vx_u8m2(v,i,vl);}

    static inline void store(elem_t* p, vector_t v, size_t vl) 
        {return __riscv_vse16_v_i16m4(p,v,vl);}

    static inline bool any_greater(vector_t v1, elem_t* p, elem_t g, size_t vl) 
    {
        vector_t v2    = __riscv_vle16_v_i16m4(p, vl);
        v2             = __riscv_vadd_vx_i16m4(v2, g, vl);
        vbool4_t mask = __riscv_vmsgt_vv_i16m4_b4(v1, v2, vl);
        long first_idx = __riscv_vfirst_m_b4(mask,vl);
        return (first_idx != -1);      
    }
    
    static inline void print_score(vector_t v, size_t vl) {
        std::vector<elem_t> aux(vl,0);
        __riscv_vse16_v_i16m4(aux.data(),v,vl);
        for (elem_t s : aux) std::cout << s << " ";
        std::cout << std::endl;
    }
};


template <> struct rvv_traits<int16_t, 8> 
{
    using vector_t = vint16m8_t;    
    using elem_t   = int16_t;    
    using index_v_t= vuint8m4_t;
    using index_t  = uint8_t;

    static inline size_t setvl(size_t n) 
        {return __riscv_vsetvl_e16m8(n);}
    
    static inline vector_t load(elem_t* p, size_t vl) 
        {return __riscv_vle16_v_i16m8(p,vl);}
    
    static inline vector_t move(elem_t v, size_t vl)  
        {return __riscv_vmv_v_x_i16m8(v,vl);}

    static inline vector_t slideup_zero(vector_t v, size_t offset, size_t vl) {
        vector_t zeros = __riscv_vmv_v_x_i16m8(0,vl);
        return __riscv_vslideup_vx_i16m8(zeros, v, offset, vl);}

    static inline vector_t loxe(const elem_t* p, index_v_t i, size_t vl)
        {return __riscv_vloxei8_v_i16m8(p, i, vl);}

    static inline index_v_t ldse(const index_t* p, size_t stride, size_t vl) 
        {return __riscv_vlse8_v_u8m4(p,static_cast<ptrdiff_t>(stride),vl);}

    static inline vector_t add(vector_t v1, vector_t v2, size_t vl) 
        {return __riscv_vadd_vv_i16m8(v1,v2,vl);}

    static inline vector_t add(vector_t v, elem_t i, size_t vl) 
        {return __riscv_vadd_vx_i16m8(v,i,vl);}

    static inline vector_t addsat(vector_t v1, vector_t v2, size_t vl)
        {   vector_t sum = __riscv_vsadd_vv_i16m8(v1,v2,vl);
            return __riscv_vmax_vx_i16m8(sum,0,vl);}

    static inline vector_t max(vector_t v1, vector_t v2, size_t vl)
        {   v1 = __riscv_vmax_vv_i16m8(v1,v2,vl);
            return __riscv_vmax_vx_i16m8(v1,0,vl);}

    static inline long max_idx(vector_t v1, elem_t val, size_t vl) {
        vbool2_t max_mask = __riscv_vmseq_vx_i16m8_b2(v1,val,vl);
        return __riscv_vfirst_m_b2(max_mask, vl);}

    static inline elem_t max(vector_t v, size_t vl) {
            vint16m1_t red_dest = __riscv_vmv_v_x_i16m1(INT16_MIN,1);
            red_dest = __riscv_vredmax_vs_i16m8_i16m1(v,red_dest,vl);
            return __riscv_vmv_x_s_i16m1_i16(red_dest);}

    static inline vector_t sll(vector_t v, size_t i, size_t vl) 
        {return __riscv_vsll_vx_i16m8(v,i,vl);}

    static inline index_v_t sll(index_v_t v, size_t i, size_t vl)
        {return __riscv_vsll_vx_u8m4(v,i,vl);}

    static inline void store(elem_t* p, vector_t v, size_t vl) 
        {return __riscv_vse16_v_i16m8(p,v,vl);}

    static inline bool any_greater(vector_t v1, elem_t* p, elem_t g, size_t vl) 
    {
        vector_t v2    = __riscv_vle16_v_i16m8(p, vl);
        v2             = __riscv_vadd_vx_i16m8(v2, g, vl);
        vbool2_t mask = __riscv_vmsgt_vv_i16m8_b2(v1, v2, vl);
        long first_idx = __riscv_vfirst_m_b2(mask,vl);
        return (first_idx != -1);      
    }
    
    static inline void print_score(vector_t v, size_t vl) {
        std::vector<elem_t> aux(vl,0);
        __riscv_vse16_v_i16m8(aux.data(),v,vl);
        for (elem_t s : aux) std::cout << s << " ";
        std::cout << std::endl;
    }
};


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