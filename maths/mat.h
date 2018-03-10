#ifndef MAT_H
#define MAT_H

#include "maths_core.h"
#include "vec.h"

#include <array>
#include <initializer_list>
#include <assert.h>
#include <cmath>
#include <type_traits>

#include <iostream>

#ifdef MATHS_ENABLE_ERROR_CHECKING
#include <sstream>
#endif

namespace cwg {
namespace maths {

template<typename T, size_t NROW, size_t NCOL>
class mat {
    std::array<T, NROW * NCOL> m_data;
public:
    mat() {}
    mat(std::array<T, NROW * NCOL> data) : m_data(data) {}
    mat(std::initializer_list<T> list) { 
        if(list.size() == NROW * NCOL) {
            size_t mi = 0;
            for(auto it = list.begin(); it != list.end(); ++it) { m_data[mi] = *it; mi++; }
        }
    }
    mat(T val) { fill(val); }

    constexpr T& at(size_t i) const
    {
    #ifdef MATHS_ENABLE_ERROR_CHECKING
        if(i >= NROW * NCOL) {
            std::stringstream ss;
            ss << "Maths exception from matrix: attempted to access index " << i << " when maximum is " << NROW * NCOL - 1;
            std::string ret = ss.str();
            throw std::range_error(ret);
        }
    #endif
        return const_cast<T&>(m_data[i]);

    }
    inline void insert(size_t i, T item) { 
    #ifdef MATHS_ENABLE_ERROR_CHECKING
        if(i >= NROW * NCOL) {
            std::stringstream ss;
            ss << "Maths exception from matrix: attempted to insert at index " << i << " when maximum is " << NROW * NCOL;
            std::string ret = ss.str();
            throw std::range_error(ret);
        }
    #endif
        m_data[i] = item;
    }

    constexpr size_t size() { return NROW * NCOL; } const
    constexpr size_t row_size() { return NROW; } const
    constexpr size_t column_size() { return NCOL; } const
    constexpr std::type_info type() { return typeid(T); } const
     
    /*
    Look into SSE instructions
    */

    #include "mat.tpp"

    //Operators
    T& operator[](size_t i) const { return at(i); }
    //scalar operators
    friend mat operator+(mat<T, NROW, NCOL>& lhs, const T& rhs) { return lhs.add(rhs); }
    friend mat operator-(mat<T, NROW, NCOL>& lhs, const T& rhs) { return lhs.subtract(rhs); }
    friend mat operator*(mat<T, NROW, NCOL>& lhs, const T& rhs) { return lhs.multiply(rhs); }
    friend mat operator/(mat<T, NROW, NCOL>& lhs, const T& rhs) { return lhs.divide(rhs); }

    mat& operator+=(const T& other) { *this = this->add(other); return *this; }
    mat& operator-=(const T& other) { *this = this->subtract(other); return *this; }
    mat& operator*=(const T& other) { *this = this->multiply(other); return *this; }
    mat& operator/=(const T& other) { *this = this->divide(other); return *this; }
    //matrix-matrix operations
    friend mat operator+(mat<T, NROW, NCOL>& lhs, const mat<T, NROW, NCOL>& rhs) { return lhs.add(rhs); }
    friend mat operator-(mat<T, NROW, NCOL>& lhs, const mat<T, NROW, NCOL>& rhs) { return lhs.subtract(rhs); }
    friend mat operator*(mat<T, NROW, NCOL>& lhs, const mat<T, NROW, NCOL>& rhs) { return lhs.multiply(rhs); }

    mat& operator+=(const mat<T, NROW, NCOL>& other) { *this = this->add(other); return *this; }
    mat& operator-=(const mat<T, NROW, NCOL>& other) { *this = this->subtract(other); return *this; }
    mat& operator*=(const mat<T, NROW, NCOL>& other) { *this = this->multiply(other); return *this; }
    //matrix-vector operations
    friend vec<T, NROW> operator*(mat<T, NROW, NCOL>& lhs, const vec<T, NROW>& rhs) { return lhs.multiply(rhs); } 
};

/* // was extremely tired when i wrote this 
template<typename T1, size_t LHS_ROW, size_t LHS_COL, typename T2, size_t RHS_ROW, size_t RHS_COL>
constexpr bool multipliable(mat<T1, LHS_ROW, LHS_COL>& lhs, mat<T2, RHS_ROW, RHS_COL>& rhs)
{ return (lhs.row_size == rhs.column_size() && std::is_arithmetic<T1>::value == true && std::is_arithmetic<T2>::value == true) ? true : false; }

template<typename T1, typename T2>
constexpr bool multipliable(size_t lhs_row, size_t rhs_col)
{ return (lhs_row == rhs_col && std::is_arithmetic<T1>::value == true && std::is_arithmetic<T2>::value == true) ? true : false; }

template<typename T, size_t LHS_ROW, size_t LHS_COL, size_t RHS_ROW, size_t RHS_COL>
std::enable_if<multipliable<T, T>(LHS_ROW, RHS_COL), mat<T, LHS_ROW, RHS_COL>
operator*(mat<T, LHS_ROW, LHS_COL>& lhs, mat<T, RHS_ROW, RHS_COL>& rhs) { return lhs.multiply(rhs); }
*/

//Type aliased
template<typename T>
using mat2 = mat<T, 2, 2>;

template<typename T>
using mat3 = mat<T, 3, 3>;

template<typename T>
using mat4 = mat<T, 4, 4>;

}
}
#endif