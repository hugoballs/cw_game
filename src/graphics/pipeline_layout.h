#ifndef PIPELINE_LAYOUT_H
#define PIPELINE_LAYOUT_H

#include <vulkan/vulkan.hpp>

namespace cwg {
namespace graphics {

class pipeline_layout {
    vk::PipelineLayout m_handle;
    vk::Device m_device;

    void create(vk::DescriptorSetLayout *layout);
    void destroy();
public:
    pipeline_layout() {}
    pipeline_layout(vk::Device dev, vk::DescriptorSetLayout *layouts);
    ~pipeline_layout();

    inline vk::PipelineLayout get() { return m_handle; }
    inline void reset() { destroy();}
    //inline void reset(vk::Format format) { destroy(); create(format);  }      //dangerous
    inline void reset(vk::Device dev, vk::DescriptorSetLayout *layouts) { destroy(); m_device = dev; create(layouts); }
};

}
}

#endif