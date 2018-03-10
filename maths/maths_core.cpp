#include "maths_core.h"

#include <cmath>

namespace cwg {
namespace maths {

double to_radians(double degrees)
{
    return  degrees * M_PIl / 180.0;
}

double to_degress(double radians)
{
    return  radians * 180.0 / M_PIl;
}


}
}