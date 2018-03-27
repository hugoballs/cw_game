#ifndef MATHS_CORE_H
#define MATHS_CORE_H

/*  Usage Notes:
    -operations that their arguments by reference; it is up to the implementor to make sure it is not modified.
    -matrices are stored in row-major order. Do not provide them in column-major or results will be different.
    -matrices use column vectors, which means they must be multiplied on from right to left.
    -all angles are expected to be provided in radians
Options: 
    -define MATHS_ENABLE_ERROR_CHECKING for error checking
*/

#ifndef NDEBUG

#ifndef MATHS_ENABLE_ERROR_CHECKING
#define MATHS_ENABLE_ERROR_CHECKING
#endif

#ifndef MATHS_ENABLE_ASSISTANCE
#define MATHS_ENABLE_ASSISTANCE
#endif

#endif

namespace cwg {
namespace maths {

    enum class axis { x, y, z };

    //conversions
    double to_radians(double degrees);
    double to_degrees(double radians);
}
}

#endif