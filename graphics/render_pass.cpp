#include "render_pass.h"

namespace cwg {
namespace graphics {

render_pass::render_pass(vk::Device dev, vk::Format format) : m_device(dev)
{
    create(format);
}

render_pass::~render_pass()
{
    destroy();
}

void render_pass::create(vk::Format format)
{
    if(m_device == vk::Device()) { throw std::runtime_error("cannot create rendere pass if there is no device."); }
    //attachments
	vk::AttachmentDescription attach_desc = {
		{},
		format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	};
	
	vk::AttachmentReference attach_ref = { 0, vk::ImageLayout::eColorAttachmentOptimal };		// 0 defines the output location in the fragment shader

	//subpass
	vk::SubpassDescription subpass_desc = {
		{},
		vk::PipelineBindPoint::eGraphics,
		{},																						//input attachment count
		{},																						//input attachment reference
		1,																						//color attachment count
		&attach_ref,																			//color attachment references
		{},																						//resolve attachment
		{},																						//depth stencil attachment
		{},																						//preserve attachment count
		{}																						//preserve attachments
	};

	//subpass dependency for drawing synchronisation
	vk::SubpassDependency dependency = {
		{},																					//src subpass, leave as ~0U OR VK_SUBPASS_EXTERNAL in vulkan-1.h
		0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,										//src subpass
		vk::PipelineStageFlagBits::eColorAttachmentOutput,										//dst subpass
		{},																						//access src
		vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,	//access dst
		{}
	};

	//create
	vk::RenderPassCreateInfo create_info = {
		{},
		1,
		&attach_desc,
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