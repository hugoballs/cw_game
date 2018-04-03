#include "window.h"
#include "../logger.h"

#include <assert.h>
#include <array>

namespace cwg {

window::window() : log("window", "log/window.log", {})
{
	if (!glfwInit()) {
		log << "GLFW failed to initialise." ;
		throw std::runtime_error("GLFW failed to initialise.");
	}
}

window::~window()
{
}

std::vector<const char*> getRequiredExtensions()
{
	unsigned int inCount;
	const char **in = glfwGetRequiredInstanceExtensions(&inCount);
	std::vector<const char*> out;
	out.reserve(inCount);

	for (unsigned int i = 0; i < inCount; i++) {
		out.push_back(in[i]);
	}

	return out;
}

void window::create_window(uint32_t width, uint32_t height, const char * name)
{
	if (glfwVulkanSupported()) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		m_window.reset(glfwCreateWindow(width, height, name, nullptr, nullptr));
		log << "Window created.";
	}
	make_current();
}

void window::create_surface(vk::Instance instance)
{
	log << "Required GLFW extensions: ";
	uint32_t count;
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);
	for(uint32_t i = 0; i < count; i++) {
		log << extensions[i];
	}
	VkSurfaceKHR temp;
	VkResult res = glfwCreateWindowSurface(instance, m_window.get(), nullptr, &temp);
	if (res != VK_SUCCESS) {
		log << "Error: failed to create window surface. Result: " << res;
		throw std::runtime_error("Failed to create window surface.");
	}
	else {
		log << "Window: created surface" ;
		m_surface = temp;
	}
}

void window::destroy_surface(vk::Instance instance)
{
	instance.destroySurfaceKHR(m_surface);
}

vk::SurfaceKHR window::get_surface()
{
	return m_surface;
}


void window::create_swapchain()
{
	assert(m_device != vk::Device() || m_physical_device != vk::PhysicalDevice() || m_presentation_queue != vk::Queue() || p_presentation_queue_info != nullptr);
	
	//image format
	std::vector<vk::SurfaceFormatKHR> formats;
	uint32_t selected_format_index = 0;

	try {
		formats = m_physical_device.getSurfaceFormatsKHR(m_surface);
	}
	catch (const std::exception& e) {
		log << "failed to retrieve surface formats: " << e.what() ;
	}

	if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {	//if undefined set it to default
		formats[0].format = vk::Format::eB8G8R8A8Unorm;
		formats[0].colorSpace = vk::ColorSpaceKHR::eExtendedSrgbNonlinearEXT;
		//atm forcing format to produce results that don't look terrible
		//TODO: make format selection algorithm
	}
	else {
		bool found = false;
		for(uint32_t i = 0; i < formats.size(); i++) {
			if(formats[i].format == vk::Format::eB8G8R8A8Unorm) {
				selected_format_index = i;
				found = true;
				break;
			}
		}
		if(!found) {
			selected_format_index = formats.size() - 1;	//select last //temp
		}
	}
	log << "got number of surface formast: " << formats.size();

	//present modes
	std::vector<vk::PresentModeKHR> present_modes;
	uint32_t selected_present_index = 0;

	try {
		present_modes = m_physical_device.getSurfacePresentModesKHR(m_surface);
	}
	catch (const std::exception& e) {
		log << "Window: failed to retrieve present modes: " << e.what() ;
	}
	log << "got present modes.";

	//TODO: add support/warnings for other modes
	for (int i = 0; i < present_modes.size(); i++) {
		if (present_modes[i] == vk::PresentModeKHR::eMailbox) {
			selected_present_index = i;
			break; //best
		}
		else if (present_modes[i] == vk::PresentModeKHR::eFifo) {
			selected_present_index = i; //ok
		}
		else if (present_modes[i] == vk::PresentModeKHR::eImmediate) {
			selected_present_index = i;
		}
	}

	//surface capabilities
	vk::SurfaceCapabilitiesKHR surface_capabilites;
	vk::Extent2D extent;
	uint32_t image_count;
	vk::ImageUsageFlags image_flags;

	try {
		surface_capabilites = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
	}
	catch (const std::exception& e) {
		log << "Window: failed to retrieve present modes: " << e.what() ;
	}
	log << "retrieved surface capabilites." ;

	//extent
	//some window managers do this
	if (surface_capabilites.currentExtent.width == std::numeric_limits<uint32_t>::max() || surface_capabilites.currentExtent.height == std::numeric_limits<uint32_t>::max()) {
		extent.setWidth(std::max(surface_capabilites.minImageExtent.width, surface_capabilites.maxImageExtent.width));
		extent.setHeight(std::max(surface_capabilites.minImageExtent.height, surface_capabilites.maxImageExtent.height));
	}
	else {
		extent = surface_capabilites.currentExtent;
	}

	//image count + format
	image_count = 2;
	//if max is 0, it means unlimited
	if (surface_capabilites.minImageCount < 3 && surface_capabilites.maxImageCount >= 3 || surface_capabilites.maxImageCount == 0) {	//triple buffering
		image_count = 3;
	}

	image_flags = vk::ImageUsageFlagBits::eColorAttachment; //its purpose

	//create swapchain
	const vk::SwapchainCreateInfoKHR create_info = { 
		{},
		m_surface,
		image_count,
		formats[selected_format_index].format,
		formats[selected_format_index].colorSpace,
		extent,
		1,
		image_flags,
		vk::SharingMode::eExclusive,
		p_presentation_queue_info->queue_family,
		p_presentation_queue_info->queue_indices.data(),
		surface_capabilites.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		present_modes[selected_present_index],
		true,
		vk::SwapchainKHR() };
	try {
		m_swapchain = m_device.createSwapchainKHR(create_info);
	}
	catch (const std::exception &e) {
		log << "failed to create swapchain: " << e.what() ;
		throw std::runtime_error("see log");
	}

	log << "swapchain created." ;
	//store
	m_image_format = formats[selected_format_index].format;
	m_image_extent = extent;
	//retrieve image handles
	try {
		m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);
	}
	catch(const std::exception& e) {
		log << "failed to retrieve swapchain images";
		throw std::runtime_error("see log.");
	}
}

void window::destroy_swapchain()
{
    m_device.waitIdle();
    for(auto iv: m_swapchain_image_views) {
        m_device.destroyImageView(iv);
    }
	m_device.destroySwapchainKHR(m_swapchain, nullptr);
}

void window::create_image_views()
{
	m_swapchain_image_views.resize(m_swapchain_images.size());
	vk::ComponentMapping components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity };
	vk::ImageSubresourceRange subrange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

	for (size_t i = 0; i < m_swapchain_images.size(); i++) {
		vk::ImageViewCreateInfo create_info = { {}, m_swapchain_images[i], vk::ImageViewType::e2D, m_image_format, components, subrange };
		try {
			m_swapchain_image_views[i] = m_device.createImageView(create_info, nullptr);
		}
		catch (const std::exception& e) {
			log << "failed to create image view " << i << ":" << e.what() ;
			throw std::runtime_error("see log.");
		}
	}
	log << "create image views" ;
}

	void window::create_framebuffers(vk::RenderPass render_pass, vk::ImageView depth_view)
	{
		vk::Extent2D extent = m_image_extent;
		auto& image_views = m_swapchain_image_views;
		size_t count = image_views.size();
		m_framebuffers.resize(count);

		for (int i = 0; i < count; i++) {
			std::array<vk::ImageView, 2> attachments = { image_views[i], depth_view };
			vk::FramebufferCreateInfo info = {{}, render_pass, static_cast<uint32_t>(attachments.size()), attachments.data(), extent.width, extent.height, 1};
			try {
				m_framebuffers[i] = m_device.createFramebuffer(info);
			}
			catch (const std::exception &e) {
				log << "failed to create framebuffer:" << e.what() ;
				throw std::runtime_error("failed to create framebuffer.");
			}
		}
	}

	void window::destroy_framebuffers()
	{
		for (int i = 0; i < m_framebuffers.size(); i++) {
			m_device.destroyFramebuffer(m_framebuffers[i]);
		}
	}


    bool window::has_resized()
    {
        static bool init = false;
        static int lwidth = 0;
        static int lheight = 0;
        if(!init) {
            glfwGetWindowSize(m_window.get(), &lwidth, &lheight);
        }
        int nwidth, nheight;
        glfwGetWindowSize(m_window.get(), &nwidth, &nheight);
        if(nwidth != lwidth || nheight != lheight)
            return true;
        else
            return false;
    }

} //end namespace