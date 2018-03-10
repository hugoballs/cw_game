#include "pipeline_layout.h"
#include <iostream>
namespace cwg {
namespace graphics {

pipeline_layout::pipeline_layout(vk::Device dev, std::vector<vk::DescriptorSetLayout>& layouts) : m_device(dev)
{
    create(layouts);
}

pipeline_layout::~pipeline_layout()
{
    destroy();
}

void pipeline_layout::create(std::vector<vk::DescriptorSetLayout>& layouts)
{
    //create
    std::cout << "layout set size(): " << layouts.size();
	vk::PipelineLayoutCreateInfo layout_info = { {}, {}, {}, {}, {} };										//TODO: fill
	try {
		m_handle = m_device.createPipelineLayout(layout_info, nullptr);
	}
	catch (...) {
		throw std::runtime_error("failed to create pipeline layout");
	}
}

void pipeline_layout::destroy()
{
    //note: no safety is provided if object is in use
    if(m_device != vk::Device() && m_handle != vk::PipelineLayout()) {
        m_device.destroyPipelineLayout(m_handle);
    }
    m_handle = vk::PipelineLayout(); 
}

}
}