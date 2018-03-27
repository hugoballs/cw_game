#ifndef BUFFER_BASE_H
#define BUFFER_BASE_H

#include <vulkan/vulkan.hpp>

namespace cwg {
namespace graphics {

//TODO: make this a template
class buffer_base {                                                 //buffer base class
protected:
    vk::BufferUsageFlags m_type_flags;
    vk::MemoryPropertyFlags m_allocation_flags;

    vk::Buffer m_handle;
    vk::DeviceMemory m_device_memory;
    vk::Device m_device;

    void create(vk::DeviceSize size_in_bytes);
    void destroy();
    void allocate(vk::PhysicalDevice p_dev);
    void deallocate();

public:
    buffer_base(vk::BufferUsageFlags type, vk::MemoryPropertyFlags alloc) : m_type_flags(type), m_allocation_flags(alloc) {}
    ~buffer_base() {}
    inline vk::Buffer get() const { return m_handle; }
    inline vk::BufferUsageFlags type() const { return m_type_flags; }
};

}
}

#endif