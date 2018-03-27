#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.hpp>

namespace cwg {
namespace graphics {

class render_pass {
    vk::RenderPass m_handle;
    vk::Device m_device;

    void create(vk::Format format);
    void destroy();
public:
    render_pass() {}
    render_pass(vk::Device dev, vk::Format format);
    ~render_pass();

    inline vk::RenderPass get() { return m_handle; }
    inline void reset() { destroy();}
    //inline void reset(vk::Format format) { destroy(); create(format);  }      //dangerous
    inline void reset(vk::Device dev, vk::Format format) { destroy(); m_device = dev; create(format); }
};

}
}
#endif