#include "descriptor_set.h"

#include <map>

namespace cwg {
namespace graphics {

descriptor_set::descriptor_set(vk::Device dev, vk::DescriptorPool pool, std::vector<descriptor> descriptors) :
    log("descriptor_set", {}),
    m_device(dev), m_pool(pool), m_descriptors(descriptors)
{
    if(!m_descriptors.empty()) {
        create_layout();
        allocate();
        configure();
    } else {
        m_state = descriptor_set_state::empty;
    }
}

descriptor_set::~descriptor_set()
{
    destroy_layout();
    deallocate();
}

void descriptor_set::create_layout()
{
    if(m_state != descriptor_set_state::empty) {
        /*std::map<uint32_t, uint32_t> bindings;    //binding num, amount of descriptors per binding
        for(const auto& desc : m_descriptors) {
            bindings[desc.binding] += 1;
        }*/ //hardcoded for now

        vk::DescriptorSetLayoutBinding b = { m_descriptors[0].binding, m_descriptors[0].type, 1, m_descriptors[0].stage, {} };

        vk::DescriptorSetLayoutCreateInfo create_info = { {}, 1, &b };
        try {
            m_layout = m_device.createDescriptorSetLayout(create_info);
        }
        catch(std::exception& e) {
            log << "failed to create descriptor set layout: " << e.what();
            throw;
        }
        m_state = descriptor_set_state::layout_made;
    }
}

void descriptor_set::destroy_layout()
{
    if(m_layout != vk::DescriptorSetLayout() && m_device != vk::Device()) {
        m_device.destroyDescriptorSetLayout(m_layout);
    }
}

void descriptor_set::allocate()
{
    vk::DescriptorSetAllocateInfo alloc_info = { m_pool, 1, &m_layout };
    try {
        std::vector<vk::DescriptorSet> temp = m_device.allocateDescriptorSets(alloc_info);
        if(temp.size() > 1) { throw std::runtime_error("invalid number of desciptor sets: descriptor_set::allocate()"); }
        m_handle = temp[0];
    }
    catch(std::exception& e) {
        log << "Failed to allocate descriptor set";
        throw;
    }
    m_state = descriptor_set_state::allocated;
}

void descriptor_set::deallocate()
{
    if(m_device != vk::Device() && m_handle != vk::DescriptorSet() && m_pool != vk::DescriptorPool()) {
        //m_device.freeDescriptorSets(m_pool, m_handle);    //missing flag bit for descriptor pool create info means this is invalid
    }
}

void descriptor_set::configure()
{
    //TODO: make loop for all potential buffers
    vk::DescriptorBufferInfo buf_info = { m_descriptors[0].buffer, {}, m_descriptors[0].size };

    vk::WriteDescriptorSet write_info = { m_handle, m_descriptors[0].binding, 0, 1, m_descriptors[0].type, {}, &buf_info, {} };
    try {
        m_device.updateDescriptorSets( { write_info}, {});
    }
    catch(std::exception& e) {
        log << "failed to configure descriptor set";
        throw;
    }
}

/*void set_descriptor(descriptor desc)
{
    m_state = descriptor_set_state::modified;
}*/

void descriptor_set::update()
{
    if(m_state == descriptor_set_state::modified) {
        deallocate();
        destroy_layout();
        create_layout();
        allocate();
        configure();
    }
}

vk::DescriptorSetLayout descriptor_set::get_layout()
{
    if(m_state == descriptor_set_state::allocated) {
        return m_layout;
    }
    else if(m_state == descriptor_set_state::empty) {
        log << "warning: attempting to access empty descriptor set layout.";
        return m_layout;
    }
    else {
        throw vk::DescriptorSetLayout();
    }
}

}
}