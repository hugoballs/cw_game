#ifndef VEC_H
#define VEC_H

#include "maths_core.h"

#include <array>
#include <initializer_list>
#include <cmath>

#ifdef MATHS_ENABLE_ERROR_CHECKING
#include <sstream>
#endif

namespace cwg {
namespace maths {

template <typename T, size_t N>
class vec {
    std::array<T, N> m_data;

public:
    vec() {}
    vec(std::array<T, N> components) : m_data(components) {}
    vec(std::initializer_list<T> list) { 
        if(list.size() == N) {
            size_t mi = 0;
            for(auto it = list.begin(); it != list.end(); ++it) { m_data[mi] = *it; mi++; }
        }
    }

    //TODO: implement exception handling
    constexpr T& at(size_t i) const
    {
    #ifdef MATHS_ENABLE_ERROR_CHECKING
        if(i >= N) {
            std::stringstream ss;
            ss << "Maths out_of_range exception from vector: attempted to access index " << i << " when maximum is " << N;
            std::string ret = ss.str();
            throw std::range_error(ret);
        }
    #endif
        return const_cast<T&>(m_data[i]); 
    }

    inline void insert(size_t i, T item)
    { 
    #ifdef MATHS_ENABLE_ERROR_CHECKING
        if(i >= N) {
            std::stringstream ss;
            ss << "Maths out_of_range exception from vector: attempted to access index " << i << " when maximum is " << N - 1;
            std::string ret = ss.str();
            throw std::range_error(ret);
        }
    #endif
        m_data[i] = item;
    }

    constexpr size_t size() const { return N; }

    #include "vec.tpp"

    /* Operators */
    T& operator[](size_t i) const { return at(i); } //access
    /* scalar operations */
    friend vec operator+(vec<T, N>& lhs, const T& rhs) { return lhs.add(rhs); }
    friend vec operator-(vec<T, N>& lhs, const T& rhs) { return lhs.subtract(rhs); }
    friend vec operator*(vec<T, N>& lhs, const T& rhs) { return lhs.multiply(rhs); }
    friend  vec operator/(vec<T, N>& lhs, const T& rhs) { return lhs.divide(rhs); }

    vec& operator+=(const T& other) { *this = this->add(other); return *this; }
    vec& operator-=(const T& other) { *this = this->subtract(other); return *this; }
    vec& operator*=(const T& other) { *this = this->multiply(other); return *this; }
    vec& operator/=(const T& other) { *this = this->divide(other); return *this; }
    /* vector operations */
    friend vec operator+(vec<T, N>& lhs, const vec<T, N>& rhs) { return lhs.add(rhs); }
    friend vec operator-(vec<T, N>& lhs, const vec<T, N>& rhs) { return lhs.subtract(rhs); }
    friend vec operator*(vec<T, N>& lhs, const vec<T, N>& rhs) { return lhs.multiply(rhs); }
    friend vec operator/(vec<T, N>& lhs, const vec<T, N>& rhs) { return lhs.divide(rhs); }

    vec& operator+=(const vec<T, N>& other) { *this = this->add(other); return *this; }
    vec& operator-=(const vec<T, N>& other) { *this = this->subtract(other); return *this; }
    vec& operator*=(const vec<T, N>& other) { *this = this->multiply(other); return *this; }
    vec& operator/=(const vec<T, N>& other) { *this = this->divide(other); return *this; }
};

//More operators


//Type aliases
template<typename T>
using vec2 = vec<T, 2>;

template<typename T>
using vec3 = vec<T, 3>;

template<typename T>
using vec4 = vec<T, 4>;

}
}

#endif