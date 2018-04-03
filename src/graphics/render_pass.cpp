#include "render_pass.h"

#include <array>

namespace cwg {
namespace graphics {

render_pass::render_pass(vk::Device dev, vk::Format colour_format, vk::Format depth_format) : m_device(dev)
{
    create(colour_format, depth_format);
}

render_pass::~render_pass()
{
    destroy();
}

void render_pass::create(vk::Format colour_format, vk::Format depth_format)
{
    if(m_device == vk::Device()) { throw std::runtime_error("cannot create rendere pass if there is no device."); }
    //attachments
	std::array<vk::AttachmentDescription, 2> attach_desc_arr;
	attach_desc_arr[0] = {
		{},
		colour_format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,						//load op
		vk::AttachmentStoreOp::eStore,						//store op
		vk::AttachmentLoadOp::eDontCare,					//stencil load op
		vk::AttachmentStoreOp::eDontCare,					//stencil store op
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	};
	attach_desc_arr[1] = {
		{},
		depth_format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal
	};

	vk::AttachmentReference colour_attach_ref = { 0, vk::ImageLayout::eColorAttachmentOptimal };		// 0 defines the output location in the fragment shader
	vk::AttachmentReference depth_attach_ref = { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

	//subpass
	vk::SubpassDescription subpass_desc = {
		{},
		vk::PipelineBindPoint::eGraphics,
		{},																						//input attachment count
		{},																						//input attachment reference
		1,																						//color attachment count
		&colour_attach_ref,																		//color attachment references
		{},																						//resolve attachment
		&depth_attach_ref,																		//depth stencil attachment
		{},																						//preserve attachment count
		{}																						//preserve attachments
	};

	//subpass dependency for drawing synchronisation
	vk::SubpassDependency dependency = {
		{},																					//src subpass, leave as ~0U OR VK_SUBPASS_EXTERNAL in vulkan-1.h
		0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,										//src subpass
		vk::PipelineStageFlagBits::eColorAttachmentOutput,										//dst subpass
		{},																					//access src
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,	//access dst
		{}
	};

	//create
	vk::RenderPassCreateInfo create_info = {
		{},
		static_cast<uint32_t>(attach_desc_arr.size()),
		attach_desc_arr.data(),
		1,
		&subpass_desc,
		1,
		&dependency
	};

	try {
		m_handle = m_device.createRenderPass(create_info, nullptr);
	}
	catch(...) {
		throw std::runtime_error("error: failed to create render pass: ");
	}
}

void render_pass::destroy()
{
    //note: no safety is provided if object is in use
    if(m_device != vk::Device() && m_handle != vk::RenderPass()) {
        m_device.destroyRenderPass(m_handle);
    }
    m_handle = vk::RenderPass(); 
}

}
}