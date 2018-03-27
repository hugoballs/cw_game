#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "buffers/uniform_buffer.h"
#include "../logger.h"

namespace cwg {
namespace graphics {

struct descriptor {
    uint32_t binding;
    vk::DescriptorType type;
    vk::ShaderStageFlagBits stage;
    vk::Buffer buffer;
    vk::DeviceSize size;
};

class descriptor_set {
    cwg::logger log;
    enum class descriptor_set_state {
        no_init,
        empty,                                      //null
        layout_made,                                 //layouts created
        allocated,
        configured,                                  //everything ready
        modified
    };
    descriptor_set_state m_state;

    vk::DescriptorSet m_handle;
    vk::Device m_device;
    vk::DescriptorPool m_pool;

    std::vector<descriptor> m_descriptors;
    vk::DescriptorSetLayout m_layout;
    

    void create_layout();
    void destroy_layout();
    void allocate();
    void deallocate();
    void configure();
public:
    descriptor_set() : log("descriptor_set", {}) {}
    descriptor_set(vk::Device dev, vk::DescriptorPool pool, std::vector<descriptor> descriptors);
    ~descriptor_set();

    inline vk::DescriptorSet get() { return m_handle; }
    inline void reset() { deallocate(); destroy_layout(); }
    inline void reset(vk::Device dev, vk::DescriptorPool pool, std::vector<descriptor> descriptors) {
        deallocate();
        destroy_layout();
        m_device = dev;
        m_pool = pool;
        m_descriptors = descriptors;
        if(!m_descriptors.empty()) {
            create_layout();
            allocate();
            configure();
        } else {
            m_state = descriptor_set_state::empty;
        }
    }
    void set_descriptor(descriptor desc);
    void update();
    vk::DescriptorSetLayout get_layout();
};

} 
}

#endif