#ifndef WINDOW_H
#define WINDOW_H

#include <vulkan/vulkan.hpp>
#include "../logger.h"
#include <GLFW/glfw3.h>
#include <memory>

#include "renderer.inl"

namespace cwg {

struct window_deleter {
	void operator()(GLFWwindow *window)
	{
		glfwDestroyWindow(window);
	}
};

class window {
private:
	//logger
	cwg::logger log;
	//window
	std::unique_ptr<GLFWwindow, window_deleter> m_window;

	vk::SurfaceKHR m_surface;
	vk::SwapchainKHR m_swapchain;

	vk::Device m_device;													//vulkan device handle
	vk::PhysicalDevice m_physical_device;									//vulkan physical device handle
	queue_info *p_presentation_queue_info;									//pointer to queue_info struct containing the necessary info for presenation
	vk::Queue m_presentation_queue;											//use this queue handle for presentation

	std::vector<vk::Image> m_swapchain_images;
	std::vector<vk::ImageView> m_swapchain_image_views;
	vk::Format m_image_format;
	vk::Extent2D m_image_extent;

public:
	std::vector<vk::Framebuffer> m_framebuffers;							//required for blending

	window();
	~window();
	
	inline void set_device(vk::Device device) { m_device = device; }
	inline void set_physical_device(vk::PhysicalDevice p) { m_physical_device = p; }
	inline void set_presentation_queue(vk::Queue q) { m_presentation_queue = q; }
	inline void set_presentation_queue_info(queue_info *i) { p_presentation_queue_info = i; }

	inline vk::SwapchainKHR get_swapchain() { return m_swapchain;  }
	inline vk::Format get_image_format() { return m_image_format; }
	inline vk::Extent2D get_image_extent() { return m_image_extent; }
	inline std::vector<vk::ImageView>& get_image_views() { return m_swapchain_image_views; }

	inline bool should_close() { return glfwWindowShouldClose(m_window.get()); }
	inline void make_current() { glfwMakeContextCurrent(m_window.get()); }
	inline void poll_events() { glfwPollEvents(); }

	void create_window(uint32_t width, uint32_t height, const char *name);							//deletion is handled by the unique_ptr
	void create_surface(vk::Instance instance);
	void destroy_surface(vk::Instance instance);
	vk::SurfaceKHR get_surface();
	
	void create_swapchain();
	void destroy_swapchain();

	void create_image_views();
	//void destroy_image_views();

	void create_framebuffers(vk::RenderPass render_pass);
	void destroy_framebuffers();

    bool has_resized();
};

}

#endif