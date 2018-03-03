#ifndef STAGING_BUFFER_H
#define STAGING_BUFFER_H

#include "buffer_base.h"
#include "vertex_buffer.h"

namespace cwg {
namespace graphics {

class staging_buffer : public buffer_base {
    vk::DeviceSize m_total_size = 0;
    vk::DeviceSize m_vertex_size = 0;

    void map(std::vector<float>& data, vk::DeviceSize size);
public:
    staging_buffer() : buffer_base(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) {}
    staging_buffer(vk::Device dev, vk::PhysicalDevice p_dev, std::vector<float>& data, vk::DeviceSize total_size, vk::DeviceSize vertex_size);
    ~staging_buffer();

    void copy(vertex_buffer& dst, vk::CommandPool pool, vk::Queue queue, vk::DeviceSize src_offset = 0, vk::DeviceSize dst_offset = 0);

    inline void reset() { deallocate(); destroy(); }
    inline void reset(vk::Device dev, vk::PhysicalDevice p_dev, std::vector<float>& data, vk::DeviceSize total_size, vk::DeviceSize vertex_size) {
         deallocate(); destroy(); m_device = dev; m_total_size = total_size; m_vertex_size = vertex_size; create(m_total_size); allocate(p_dev); map(data, m_total_size); }
};

}    
}

#endif