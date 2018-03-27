/* This file contains most of the definitions for functions which require template parameters */

void fill(T val)                                                        //fills all the elements of the principal diagonal: used for identity matrix;
    {
        assert(NROW == NCOL);                                               //temporary
        size_t offset = 0;
        for(size_t i = 0; i < NROW * NCOL; i++) {
            if(i  == NROW * offset + offset) {
                at(i) = val;
                offset++;
            }
            else {
                m_data[i] = 0;
            }
        }
    }

/*          extraction                  */

vec<T, NCOL> extract_from_row(size_t row_num) const
{
    vec<T, NCOL> temp;
    for(size_t j = 0; j < temp.size(); j++) {
        temp[j] = this->at(NCOL * row_num + j); 
    }
    return temp;
}

vec<T, NROW> extract_from_column(size_t col_num) const
{
    vec<T, NROW> temp;
    for(size_t i = 0; i < temp.size(); i++) {
        temp[i] = this->at(NCOL * i + col_num); 
    }
    return temp;
}

/*          data-access for APIs    */

std::array<T, NROW * NCOL> row_major_data() {
    return m_data;      //copy, never allow access to the underlying layer through the data() function of std::array
}

std::array<T, NROW * NCOL> column_major_data() {
    std::array<T, NROW * NCOL> temp;
    vec<T, NROW> col_vec;
    for(size_t i = 0; i < NCOL; i++) {
        col_vec = extract_from_column(i);
        for(size_t j = 0; j < NROW; j++) {
            temp[i * NCOL + j] = col_vec[j];
        }
    }
    return temp;
}

/*          scalar operations           */

constexpr mat<T, NROW, NCOL> add(const T& scalar) const
{
    mat<T, NROW, NCOL> temp;
    for(size_t i = 0; i < NROW * NCOL; i++) {
        temp[i] = this->at(i) + scalar;
    }
    return temp;
}

constexpr mat<T, NROW, NCOL> subtract(const T& scalar) const
{
    mat<T, NROW, NCOL> temp;
    for(size_t i = 0; i < NROW * NCOL; i++) {
        temp[i] = this->at(i) - scalar;
    }
    return temp;
}

constexpr mat<T, NROW, NCOL> multiply(const T& scalar) const
{
    mat<T, NROW, NCOL> temp;
    for(size_t i = 0; i < NROW * NCOL; i++) {
        temp[i] = this->at(i) * scalar;
    }
    return temp;
}

constexpr mat<T, NROW, NCOL> divide(const T& scalar) const
{
    mat<T, NROW, NCOL> temp;
    for(size_t i = 0; i < NROW * NCOL; i++) {
        temp[i] = this->at(i) / scalar;
    }
    return temp;
}

/*              matrix-matrix operations            */

constexpr mat<T, NROW, NCOL> add(const mat<T, NROW, NCOL>& other) const
{
    mat<T, NROW, NCOL> temp;
    for(size_t i = 0; i < NROW * NCOL; i++) {
        temp[i] = m_data[i] + other.at(i);
    }
    return temp;
}

constexpr mat<T, NROW, NCOL> subtract(const mat<T, NROW, NCOL>& other) const
{
    mat<T, NROW, NCOL> temp;
    for(size_t i = 0; i < NROW * NCOL; i++) {
        temp[i] = this->at(i) - other.at(i);
    }
    return temp;
}

//template<size_t OTHER_ROW, size_t OTHER_COL>
constexpr mat<T, NROW, NCOL> multiply(const mat<T, NROW, NCOL>& other) const
{
#ifdef MATHS_ENABLE_ERROR_CHECKING
/*
    if(this->column_size() != other.row_size()) {
        std::stringstream ss;
        ss << "Maths exception from matrix: attempted to multiply " << this->row_size() << "x" << this->column_size() << " matrix by " << other.row_size() << "x" << other.column_size() << " matrix";
        std::string ret = ss.str();
        throw std::invalid_argument(ret);
    }
    */
#endif
    //TODO: implement eintstein summation
    mat<T, NROW, NCOL> temp(0);

    size_t row_num = 0;
    size_t col_num = 0;
    vec<T, NROW> row_vec;
    vec<T, NCOL> col_vec;

    for(size_t i = 0; i < temp.size(); i++) {
        if(i % NCOL == 0) { //fill the row vector if required
            row_vec = extract_from_row(row_num);
            row_num++;
            col_num = 0;
        }
        col_vec = other.extract_from_column(col_num);

        for(size_t k = 0; k < row_vec.size(); k++) {
            //std::cout << "temp[" << i << "] += " << row_vec[k] << " * " << col_vec[k] << " = " << row_vec[k] * col_vec[k] << std::endl;
            temp[i] += row_vec[k] * col_vec[k];
        }
        col_num++;
    }
    return temp;
}

/*      matrix-vector operation(s)    */

constexpr vec<T, NROW> multiply(const vec<T, NROW>& vector) const
{
    vec<T, NROW> temp;
    vec<T, NROW> row_vec;
    size_t row_num = 0;
    for(size_t i = 0; i < temp.size(); i++) {
        row_vec = extract_from_row(row_num);
        row_num++;;
        temp[i] = row_vec.dot_product(vector);
    }
    return temp;
}

//linear transformations
//TODO: overhaul this with funnctions/mapping

void translate(vec<T, NROW>& vector)        //must have the same amount of rows as the vector
{
    //TODO: should maybe have an optional checking layer for parameter checking, example: did you mean to set w coordinate to 0?
    size_t vec_i = 0;
    for(size_t i = NCOL -1; i < NROW * NCOL; i += NCOL) { //fill all the elements of the last column with the vector parameter
        this->at(i) = vector[vec_i];
        vec_i++;
    }
}

void scale(vec<T, NROW>& vector)
{
    size_t offset = 0;
    for(size_t i = 0; i < NROW * NCOL; i += NCOL + 1) {
        this->at(i) = vector[offset];
        offset++;
    }
}

void rotate(axis ax, double angle)      //angle in radians
{
    assert(NCOL > 2 && NROW > 3);
    switch(ax) {
        case axis::x: {
            this->at(NCOL + 1) = cos(angle);
            this->at(NCOL + 2) = -sin(angle);
            this->at(2 * NCOL + 1) = sin(angle);
            this->at(2 * NCOL + 2) = cos(angle);
            break;
        }
        case axis::y: {
            this->at(0) = cos(angle);
            this->at(2) = sin(angle);
            this->at(2 * NCOL) = -sin(angle);
            this->at(2 * NCOL + 2) = cos(angle);
        }
        case axis::z: {
            this->at(0) = cos(angle);
            this->at(1) = -sin(angle);
            this->at(NCOL) = sin(angle);
            this->at(NCOL + 1) = cos(angle);
        }
    }
}

void perspective_proj(double fov, double aspect_ratio, double near_plane, double far_plane)
{
    //this->at
}