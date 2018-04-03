#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.hpp>
#include "../logger.h"
#include "window.h"
#include <memory>
#include <chrono>
#include <string>


#include "renderer.inl"
#include "render_pass.h"
#include "pipeline.h"
#include "pipeline_layout.h"
#include "descriptor_set.h"

#include "buffers/vertex_buffer.h"
#include "buffers/index_buffer.h"
#include "buffers/staging_buffer.h"
#include "buffers/uniform_buffer.h"

#include "misc/fps_counter.h"

namespace cwg {
namespace graphics {

enum class renderer_states {
	no_init,
	init
};

class renderer {
private:
	//internal
	renderer_states m_internal_state = renderer_states::no_init;
	//logger
	cwg::logger log;
	//vulkan
	vk::Instance m_instance;
	window m_window;

	vk::Device m_device;													//vulkan device handle
	vk::PhysicalDevice m_physical_device;	
	vk::Queue m_graphics_queue;												//graphics queue handle
	queue_info m_graphics_queue_info;										//struct containing queue family + queue indices
	vk::Queue m_presentation_queue;											//same but for presentation
	queue_info m_presentation_queue_info;									//same. it could have the same queue family as the graphics queue depending on the hardware

	render_pass m_primary_render_pass;
	pipeline_layout m_primary_layout;
	pipeline m_primary_pipeline;
	
	staging_buffer m_staging_buffer;
	vertex_buffer m_primary_vb;									//vertex buffer being used to draw
	index_buffer m_primary_ib;

	vk::CommandPool m_command_pool;
	vk::CommandPool m_transfer_pool;
	std::vector<vk::CommandBuffer> m_command_buffers;						//use 1 command buffer per frame (example: 3 for triple-buffering)

	vk::DescriptorPool m_descriptor_pool;
	uniform_buffer m_uniform_buffer;
	vk::DeviceSize m_uniform_buffer_size = 3 * 16 * sizeof(float);
	vk::DescriptorSet m_descriptor_set;
	vk::DescriptorSetLayout m_descriptor_layout;

	vk::Image m_tex;
	vk::DeviceMemory m_tex_mem;
	vk::ImageView m_tex_view;
	vk::Sampler m_tex_sampler;
	bool m_sampler_anistropy;

	vk::Image m_depth_image;
	vk::DeviceMemory m_depth_mem;
	vk::ImageView m_depth_view;
	vk::Format m_depth_format;

	vk::Semaphore m_render_should_begin;										//semaphores used for synchronisation in the draw() function
	vk::Semaphore m_render_has_finished;

	//functions
	void create_instance();
	void destroy_instance();
	void verify_instance_extensions(std::vector<name_and_version>& wanted, std::vector<const char*>& out);
	void verify_instance_layers(std::vector<name_and_version>& wanted, std::vector<const char*>& out);

	void create_device();
	void destroy_device();
	void verify_device_extensions(std::vector<name_and_version>& wanted, std::vector<const char*>& out);

	void create_command_pool();
	void destroy_command_pool();
	void create_transfer_pool();
	void destroy_transfer_pool();
	vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level);
	void destroy_command_buffer(vk::CommandBuffer buffer);
	void record_command_buffer(vk::CommandBuffer cmd_buffer, vk::Framebuffer framebuffer, vk::Pipeline pipeline, graphics::vertex_buffer& vb);

	void create_descriptor_pool(uint32_t max_sets = 1);
	void destroy_descriptor_pool();
	void create_descriptor_set_layout();
	void destroy_descriptor_set_layout();
	void create_descriptor_set();
	void destroy_descriptor_set();
	void update_uniform_buffer();

	void create_texture(std::string path);
	void destroy_texture();
	void create_image( vk::Image *img, vk::DeviceMemory *mem, int32_t width, int32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits mem_flags);
	void destroy_image(vk::Image *img, vk::DeviceMemory *img_mem);
	void transition_image_layout(vk::Image& img, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout);
	void create_image_view(vk::Image *img, vk::ImageView *iv, vk::Format format, vk::ImageAspectFlagBits asp_flags);
	void destroy_image_view(vk::ImageView *iv);
	void create_sampler();
	void destroy_sampler();

	void create_depth_buffer();
	void destroy_depth_buffer();
	vk::Format select_image_format(std::vector<vk::Format>&& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	void create_drawing_enviroment(graphics::vertex_buffer& vb);
	void destroy_drawing_enviroment();

	void create_swapchain();
	void clear_swapchain();
	void recreate_swapchain();

    void create_pipeline();
    void clear_pipeline();
    void recreate_pipeline();                                                    //dont know what to name yet. recreates the pipeline

	fps_counter m_fps_counter;
public:
	renderer();
	~renderer();

	void draw();

	inline bool should_close() { return m_window.should_close(); }
	inline void poll_events() { m_window.poll_events(); }
};



}
}

#endif