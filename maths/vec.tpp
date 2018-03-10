/* Contains most of the template definitions */

/* scalar operations */

constexpr vec<T, N> add(const T& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = this->at(i) + other;
    }
    return temp;    //C++17 gurarantees copy elision
}

constexpr vec<T, N> subtract(const T& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = m_data[i] - other;
    }
    return temp;
}

constexpr vec<T, N> multiply(const T& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = m_data[i] * other;
    }
    return temp;
}

constexpr vec<T, N> divide(const T& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = m_data[i] / other;
    }
    return temp;
}

/*          vector-vector operations       */

constexpr vec<T, N> add(const vec<T, N>& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = this->at(i) + other[i];
    }
    return temp;    //C++17 gurarantees copy elision
}

constexpr vec<T, N> subtract(const vec<T, N>& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = m_data[i] - other[i];
    }
    return temp;
}

constexpr vec<T, N> multiply(const vec<T, N>& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = m_data[i] * other[i];
    }
    return temp;
}

constexpr vec<T, N> divide(const vec<T, N>& other) const
{
    vec<T, N> temp;
    for(size_t i = 0; i < N; i++) {
        temp[i] = m_data[i] / other[i];
    }
    return temp;
}

/* Vector operations */

template<typename return_type = double>
constexpr return_type magnitude() const             //also called norm
{
    T sum = 0;
    for(size_t i = 0; i < this->size(); i++) {
        sum += std::pow(this->at(i), 2);
    }
    return std::sqrt(sum);
}

constexpr T dot_product(const vec<T, N>& other) const
{
    //assert(this->size() == other.size())
    T sum = 0;
    for(size_t i = 0; i < this->size(); i++) {
        sum += this->at(i) * other.at(i);
    }
    return sum;
}