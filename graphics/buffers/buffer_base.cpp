#include "buffer_base.h"

namespace cwg {
namespace graphics {

void buffer_base::create(vk::DeviceSize size_in_bytes)
{
    vk::BufferCreateInfo create_info = { {}, size_in_bytes, m_type_flags, vk::SharingMode::eExclusive, {}, {} };               //last 2 args are for concurrent mode
    try {
        m_handle = m_device.createBuffer(create_info);
    }
    catch(...) {
        throw std::runtime_error("error: failed to create vertex buffer object.");
    }
}

void buffer_base::destroy()
{
    if(m_device != vk::Device() && m_handle != vk::Buffer()) {
        m_device.destroyBuffer(m_handle);
    }
    m_handle = vk::Buffer();
}

void buffer_base::allocate(vk::PhysicalDevice p_dev)
{
    //get memory requirements
    vk::MemoryRequirements mem_req = m_device.getBufferMemoryRequirements(m_handle);
    vk::PhysicalDeviceMemoryProperties mem_props = p_dev.getMemoryProperties();
    uint32_t mem_index = std::numeric_limits<uint32_t>::max();

    for(uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if(mem_req.memoryTypeBits & (1 << i) && mem_props.memoryTypes[i].propertyFlags & (m_allocation_flags)) {
            mem_index = i;
            break;
        }
    }

    if(mem_index == std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("error: failed to find suitable memory for vertex buffer.");
    }

    vk::MemoryAllocateInfo alloc_info = {mem_req.size, mem_index };
    try {
        m_device_memory = m_device.allocateMemory(alloc_info);
    }
    catch(...) {
        throw std::runtime_error("error: failed to allocate vb memory.");
    }
    m_device.bindBufferMemory(m_handle, m_device_memory, 0);
}

void buffer_base::deallocate()
{
    if(m_device != vk::Device() && m_device_memory != vk::DeviceMemory()) {
        m_device.freeMemory(m_device_memory);
    }
    m_device_memory = vk::DeviceMemory();
}

}
}