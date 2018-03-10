#ifndef CAMERA_H
#define CAMERA_H

#include "../../maths/mat.h"

namespace cwg {
namespace graphics {

class camera {
    maths::mat4<float> m_transformation;
public:
    camera() : m_transformation(1.0f) {}

    //mat4<float> view_matrix 
};

}
}

#endif