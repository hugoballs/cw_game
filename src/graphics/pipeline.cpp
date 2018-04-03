#include "pipeline.h"

#include <fstream>

namespace cwg {
namespace graphics {

pipeline::pipeline(vk::Device dev, vk::RenderPass rp, vk::PipelineLayout lay, vk::Extent2D extent, graphics::vertex_buffer *vb) : m_device(dev)
{
    create(rp, lay, extent, vb);
}

pipeline::~pipeline()
{
    destroy();
}

void pipeline::create(vk::RenderPass rp, vk::PipelineLayout lay, vk::Extent2D extent, graphics::vertex_buffer *vb)
{
    if(m_device == vk::Device()) { throw std::runtime_error("cannot create rendere pass if there is no device."); }
    //shader stages
	vk::ShaderModule vertex_module = create_shader(R"(./resources/vert.spv)");
	vk::ShaderModule frag_module = create_shader(R"(./resources/frag.spv)");
    //for temporary cleanup
    m_shaders.push_back(vertex_module);
    m_shaders.push_back(frag_module);
	
	vk::PipelineShaderStageCreateInfo vertex_stage_info = { {}, vk::ShaderStageFlagBits::eVertex, vertex_module, "main", {} };
	vk::PipelineShaderStageCreateInfo frag_stage_info = { {}, vk::ShaderStageFlagBits::eFragment, frag_module, "main", {} };

	vk::PipelineShaderStageCreateInfo shaders[] = { vertex_stage_info, frag_stage_info };

	//input TODO: add binding desc + attribute desc
	std::vector<vk::VertexInputBindingDescription> bindings;
	vb->get_binding_descriptions(&bindings);
	std::vector<vk::VertexInputAttributeDescription> attribs;
	vb->get_attribute_descriptions(&attribs);

	vk::PipelineVertexInputStateCreateInfo vertex_input_state = { {}, static_cast<uint32_t>(bindings.size()), bindings.data(), static_cast<uint32_t>(attribs.size()), attribs.data() };

	//input assembly TODO: set flast one to true for index buffers
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state = { {}, vk::PrimitiveTopology::eTriangleList, false };

	//viewport
	vk::Viewport viewport = { 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
	vk::Rect2D scissor = { {0, 0}, extent };
	vk::PipelineViewportStateCreateInfo viewport_state = { {}, 1, &viewport, 1, &scissor };

	//rasteriser
	vk::PipelineRasterizationStateCreateInfo rasterisation_state = {
		{},
		false,																			//clamp enable
		false,																			//rasteriser discard
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eCounterClockwise,
		false,																			//depth bias
		0.0f,
		0.0f,
		0.0f,
		1.0f																			//line width
	};

	//multisampling
	vk::PipelineMultisampleStateCreateInfo multisample_state = { {}, vk::SampleCountFlagBits::e1 }; //TODO: actually look into this

	//TODO: depth + stencil testing

	//color blending TODO: implement blending
	vk::PipelineColorBlendAttachmentState colour_blend_attachment = { false, {}, {}, {}, {}, {}, {},
																	vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

	vk::PipelineColorBlendStateCreateInfo colour_blend_state = { {}, false, {}, 1, &colour_blend_attachment, {} };			//Blending is set to false for now

	//dynamic state
	vk::DynamicState dynamic_states_array[] = { vk::DynamicState::eViewport, vk::DynamicState::eLineWidth };			
	vk::PipelineDynamicStateCreateInfo dynamic_state = { {}, 2, dynamic_states_array };

	//depth stencil
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state = {
		{},
		true,
		true,
		vk::CompareOp::eLess,
		false,
		false,
		{},
		{},
		0.0f,
		1.0f
	};

	//create
	vk::Pipeline pipeline;
	vk::GraphicsPipelineCreateInfo create_info = {
		{},
		2,
		shaders,
		&vertex_input_state,
		&input_assembly_state,
		{},																	//tesselation
		&viewport_state,
		&rasterisation_state,
		&multisample_state,
		&depth_stencil_state,												// depth stencil
		&colour_blend_state,
		{},																	//TODO: implement dynamic state
		lay,
		rp
	};


	try {
		m_handle = m_device.createGraphicsPipeline(vk::PipelineCache(), create_info, nullptr);
	}
	catch (...) {
		throw std::runtime_error("error: failed to create graphics pipeline.");
	}
	
}

void pipeline::destroy()
{
    //note: no safety is provided if object is in use
    if(m_device != vk::Device() && m_handle != vk::Pipeline()) {
        m_device.destroyPipeline(m_handle);
    }
    for(auto shader: m_shaders) {
        m_device.destroyShaderModule(shader);
    }
    m_shaders.clear();
    m_handle = vk::Pipeline(); 
}

//helper

vk::ShaderModule pipeline::create_shader(std::string path)
{
	std::ifstream file(path, std::ifstream::ate | std::ifstream::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open shader file");
	}
	else {
		size_t size = (size_t)file.tellg();
		std::vector<char> buffer(size);
		file.seekg(0);
		file.read(buffer.data(), size);
		//file.close();

		vk::ShaderModuleCreateInfo create_info = { {}, buffer.size(), reinterpret_cast<const uint32_t*>(buffer.data()) };
		vk::ShaderModule shader;
		try {
			shader = m_device.createShaderModule(create_info, nullptr);
		}
		catch (...) {
			throw std::runtime_error("failed to create shader module.");
		}
		return shader;
	}
}

}
}