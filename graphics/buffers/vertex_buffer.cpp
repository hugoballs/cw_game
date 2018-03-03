#include "vertex_buffer.h"

#include <set>
#include <map>

namespace cwg {
namespace graphics {

vertex_buffer::vertex_buffer(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size, vk::DeviceSize vertex_size) :
    buffer_base(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal),
    log("vertex_buffer", "log/vb.log", {})
{
    m_device = dev;
    m_total_size = total_size;
    m_vertex_size = vertex_size;

    create(m_total_size);
    allocate(p_dev);
}

vertex_buffer::~vertex_buffer()
{
    deallocate();
    destroy();
}

void vertex_buffer::get_binding_descriptions(std::vector<vk::VertexInputBindingDescription> *desc)
{
    desc->clear();
    std::set<unsigned char> history;        //stores all the processed bindings
    for(uint32_t i = 0; i < m_attributes.size(); i++) {
        if(history.find(m_attributes[i].binding) != history.end()) {
            continue;
        }
        unsigned char element_count = 0;
        for(uint32_t j = 0; j <m_attributes.size(); j++) {
            if(m_attributes[j].binding == m_attributes[i].binding) {
                element_count += m_attributes[j].stride;
            }
        }
        desc->emplace_back(m_attributes[i].binding, element_count * sizeof(float), vk::VertexInputRate::eVertex);
        history.emplace(m_attributes[i].binding);       //append this binding to the record to avoid duplication
    }
}

void vertex_buffer::get_attribute_descriptions(std::vector<vk::VertexInputAttributeDescription> *desc)
{
    //1) get vec of unique bindings. 2) iterate through and create descs.
    std::map<unsigned char, unsigned char> offsets;         //list of binding + offset
    for(uint32_t i = 0; i < m_attributes.size(); i++) {
        offsets[m_attributes[i].binding] = 0;   //insert or assign null
    }
    
    for(uint32_t i = 0; i < m_attributes.size(); i++) {
        vk::Format stride_format;
        switch(m_attributes[i].stride) {
            case 2: stride_format = vk::Format::eR32G32Sfloat; break;
            case 3: stride_format = vk::Format::eR32G32B32Sfloat; break;
            case 4: stride_format = vk::Format::eR32G32B32A32Sfloat; break;
            default: throw std::runtime_error("error: unsupported vb attribute format.");
        }
        desc->emplace_back(static_cast<uint32_t>(m_attributes[i].location), static_cast<uint32_t>(m_attributes[i].binding), stride_format, offsets.find(m_attributes[i].binding)->second * sizeof(float));
        offsets[m_attributes[i].binding] += m_attributes[i].stride;
    }
}

}
}