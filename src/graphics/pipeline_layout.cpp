#include "pipeline_layout.h"
#include <iostream>
namespace cwg {
namespace graphics {

pipeline_layout::pipeline_layout(vk::Device dev, vk::DescriptorSetLayout *layouts) : m_device(dev)
{
    create(layouts);
}

pipeline_layout::~pipeline_layout()
{
    destroy();
}

void pipeline_layout::create(vk::DescriptorSetLayout *layout)
{
    //layouts.clear();//NOTE: ignoring layouts for now

    //create
    //Use layout again in args 2 and 3
	vk::PipelineLayoutCreateInfo layout_info = { {}, 1, layout, {}, {} };										//TODO: fill
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
