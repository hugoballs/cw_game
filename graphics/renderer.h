#ifndef RENDERER_H
#define RENDERER_H

#include <vulkan/vulkan.hpp>
#include "../logger.h"
#include "window.h"
#include <memory>

#include "renderer.inl"

namespace cwg {


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

    std::vector<vk::ShaderModule> m_primary_shaders;
	vk::RenderPass m_primary_render_pass;
	vk::PipelineLayout m_primary_layout;
	vk::Pipeline m_primary_pipeline;

    /*  Will not use since going to go for oject-oriented abstraction
	std::vector<vk::RenderPass> m_render_passes;							//vector containing all render pass handles with names
	std::vector<vk::Pipeline> m_pipelines;									//same but for pipelines
	std::vector<vk::PipelineLayout> m_pipeline_layouts;						//TODO: rethink
    */

	vk::CommandPool m_command_pool;
	std::vector<vk::CommandBuffer> m_command_buffers;						//use 1 command buffer per frame (example: 3 for triple-buffering)

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

	size_t find_render_pass(vk::RenderPass handle);							//functions for finding handle in vectors
	size_t find_pipeline(vk::Pipeline handle);
	size_t find_pipeline_layout(vk::PipelineLayout handle);

	vk::ShaderModule create_shader(std::string path);
	vk::RenderPass create_render_pass(vk::Format format);					//basic, no options yet
	void destroy_render_pass(vk::RenderPass handle);

	vk::Pipeline create_pipeline(vk::RenderPass& render_pass, vk::PipelineLayout& layout);
	void destroy_pipeline(vk::Pipeline handle);

	vk::PipelineLayout create_pipeline_layout();
	void destroy_pipeline_layout(vk::PipelineLayout handle);

	void create_command_pool();
	void destroy_command_pool();
	vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level);
	void destroy_command_buffer(vk::CommandBuffer buffer);
	void record_command_buffer(vk::CommandBuffer cmd_buffer, vk::Framebuffer framebuffer, vk::Pipeline pipeline, uint32_t vertex_count);

	void create_drawing_enviroment();
	void destroy_drawing_enviroment();

	void create_swapchain();
	void clear_swapchain();
	void recreate_swapchain();

    void create_pipeline();
    void clear_pipeline();
    void recreate_pipeline();                                                    //dont know what to name yet. recreates the pipeline
public:
	renderer();
	~renderer();

	void draw();

	inline bool should_close() { return m_window.should_close(); }
	inline void poll_events() { m_window.poll_events(); }
};



}

#endif