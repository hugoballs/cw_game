#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "../../logger.h"
#include <vector>

#include "buffer_base.h"
//#include "staging_buffer.h"

namespace cwg {
namespace graphics {

//TODO: make templates
//TODO: add safety for debug

class index_buffer : public buffer_base {
    cwg::logger log;
    vk::DeviceSize m_total_size = 0;

    struct attrib { unsigned char binding; unsigned char location; unsigned char stride; };
    std::vector<attrib> m_attributes;
public:
    index_buffer() : buffer_base(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal) , log("index_buffer", "log/ib.log", {}) {}
    index_buffer(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size);
    //vertex_buffer(staging_buffer& import_from);
    ~index_buffer();

    inline void reset() { deallocate(); destroy(); m_total_size = 0; }
    inline void reset(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size) {
         deallocate(); destroy(); m_device = dev; m_total_size = total_size; create(m_total_size); allocate(p_dev); }

    inline uint32_t size() { return static_cast<uint32_t>(m_total_size / sizeof(uint32_t)); }
    inline void set_total_size(vk::DeviceSize s) { m_total_size = s; }
    inline vk::IndexType get_index_type() { return vk::IndexType::eUint32; }
};

}
}

#endif