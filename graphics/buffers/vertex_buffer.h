#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "../../logger.h"
#include <vector>

#include "buffer_base.h"
//#include "staging_buffer.h"

namespace cwg {
namespace graphics {

//TODO: make templates
//TODO: add safety for debug

class vertex_buffer : public buffer_base {
    cwg::logger log;
    vk::DeviceSize m_total_size = 0;
    vk::DeviceSize m_vertex_size = 0;

    struct attrib { unsigned char binding; unsigned char location; unsigned char stride; };
    std::vector<attrib> m_attributes;
public:
    vertex_buffer() : buffer_base(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal) , log("vertex_buffer", "log/vb.log", {}) {}
    vertex_buffer(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size, vk::DeviceSize vertex_size);
    //vertex_buffer(staging_buffer& import_from);
    ~vertex_buffer();

    inline void reset() { deallocate(); destroy(); }
    inline void reset(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size, vk::DeviceSize vertex_size) {
         deallocate(); destroy(); m_device = dev; m_total_size = total_size; m_vertex_size = vertex_size; create(m_total_size); allocate(p_dev); }

    inline void set_attribute(unsigned char binding, unsigned char location, unsigned char stride) { m_attributes.push_back( {binding, location, stride} ); }             //no need for more than 256 attributes
    //no out of range or location duplication checking exists

    void get_binding_descriptions(std::vector<vk::VertexInputBindingDescription> *desc);
    void get_attribute_descriptions(std::vector<vk::VertexInputAttributeDescription> *desc);

    inline uint32_t size() { return static_cast<uint32_t>(m_total_size / m_vertex_size); }
    inline void set_total_size(vk::DeviceSize s) { m_total_size = s; }
    inline void set_vertex_size(vk::DeviceSize s) { m_vertex_size = s; }
};

}
}

#endif