#ifndef UNIFORM_BUFFER
#define UNIFORM_BUFFER

#include "buffer_base.h"

namespace cwg {
namespace graphics {

class uniform_buffer : public buffer_base {
    vk::DeviceSize m_total_size;

public:
    uniform_buffer() : buffer_base(vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) {}
    uniform_buffer(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size) :
    buffer_base(vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    {
        m_device = dev;
        m_total_size = total_size;

        create(m_total_size);
        allocate(p_dev);
    }

    ~uniform_buffer()
    {
        deallocate();
        destroy();
    }
    
    inline void reset() { deallocate(); destroy(); }
    inline void reset(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size) {
        deallocate();
        destroy();
        m_device = dev;
        m_total_size = total_size;
        create(m_total_size);
        allocate(p_dev);
    }

    inline size_t size() { return m_total_size; }

    #include "uniform_buffer.tpp"
};

}
}

#endif