#include <cstring>
#include <fstream>

#include "renderer.h"

namespace cwg {

renderer::renderer() : log("renderer", "log/renderer.log", {})
{
	/* Right now there is no initialisation. will definetly need vulkan support detection*/
	m_internal_state = renderer_states::init;

	create_instance();	//first
	m_window.create_window(640, 480, "window");
	m_window.create_surface(m_instance);
	create_device();
	create_swapchain();
    create_command_pool();
	create_pipeline();

	create_drawing_enviroment();
}

renderer::~renderer()
{
    destroy_drawing_enviroment();
	clear_pipeline();
    destroy_command_pool();
	clear_swapchain();
	destroy_device();
	m_window.destroy_surface(m_instance);
	destroy_instance();	//do last
}

//SEPERATOR: instance

void renderer::create_instance()
{
	std::vector<name_and_version> instance_extensions = {
		{ "VK_KHR_surface", ANY_NAV_VERSION }
	};
#ifdef _WIN32
	instance_extensions.push_back({ "VK_KHR_win32_surface", ANY_NAV_VERSION });
#elif defined __linux
	instance_extensions.push_back({ "VK_KHR_xlib_surface", ANY_NAV_VERSION });
#endif

		std::vector<name_and_version> instance_layers;
#ifndef NDEBUG
		instance_layers.push_back({ "VK_LAYER_LUNARG_standard_validation", ANY_NAV_VERSION });
#endif

    std::vector<const char*> checked_extensions;
	verify_instance_extensions(instance_extensions, checked_extensions);
    std::vector<const char*> checked_layers;
	verify_instance_layers(instance_layers, checked_layers);

	vk::ApplicationInfo app_info = { "graphicsProject", VK_MAKE_VERSION(0,1,0), "no_name", VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_0 };
	vk::InstanceCreateInfo create_info = { {}, &app_info, static_cast<uint32_t>(checked_layers.size()), checked_layers.data(), static_cast<uint32_t>(checked_extensions.size()), checked_extensions.data() };
	
	try {
		m_instance = vk::createInstance(create_info);
	}
	catch (std::exception const &e) {
		log << "Exception on instance creation: " << e.what() ;
	}
}

void renderer::destroy_instance()
{
	m_instance.destroy();
}

void renderer::verify_instance_extensions(std::vector<name_and_version>& wanted, std::vector<const char*>& out)
{
	auto available = vk::enumerateInstanceExtensionProperties();
	vk::Bool32 found = 0;
    for (const auto &i : wanted) {
        found = 0;
        for (const auto a : available) {
            if (std::strcmp(a.extensionName, i.name) == 0 && i.version == ANY_NAV_VERSION ||
                a.specVersion == i.version) {
                out.push_back(i.name);
                log << "Using instance extension: " << a.extensionName ;
                found = 1;
                break;
            } else if (std::strcmp(a.extensionName, i.name) == 0) {
                out.push_back(i.name);
                log << "Warning: instance extension version does not match" ;
                found = 1;
                break;
            }
        }
        if (!found) {
            log << "Missing instance extension: " << i.name ;
            throw std::runtime_error("Instance extension missing");
        }
    }
}

void renderer::verify_instance_layers(std::vector<name_and_version>& wanted, std::vector<const char*>& out)
{
	auto available = vk::enumerateInstanceLayerProperties();
	vk::Bool32 found = 0;

	for (const auto& i : wanted) {
		found = 0;
		for (const auto a : available) {
			if (std::strcmp(a.layerName, i.name) == 0 && i.version == ANY_NAV_VERSION || a.implementationVersion == i.version) {
				out.push_back(i.name);
				log << "Using instance layer: " << a.layerName ;
				found = 1;
				break;
			}
			else if (std::strcmp(a.layerName, i.name) == 0) {
				out.push_back(i.name);
				log << "Warning: instance layer version does not match" ;
				found = 1;
				break;
			}
		}
		if (!found) {
			log << "Missing instance layer: " << i.name ;
			throw std::runtime_error("Instance layer missing");
		}
	}
}

//SEPERATOR: device

void renderer::create_device()
{
	//physical devices
	std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();

	for (auto& device : physical_devices) { //TODO: Make a proper evalutation system
		if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu || device.getProperties().deviceType ==  vk::PhysicalDeviceType::eIntegratedGpu) {
			m_physical_device = device;
			log << "Physical Device: using " << device.getProperties().deviceName << "\n" ;
			break;
		}
	}

	//queue families
	std::vector<vk::QueueFamilyProperties> device_queues = m_physical_device.getQueueFamilyProperties();
	uint32_t gq_count = 1;
	uint32_t pq_count = 1;
	uint32_t gq_fam = std::numeric_limits<uint32_t>::max();
	uint32_t pq_fam = std::numeric_limits<uint32_t>::max();

	for (unsigned int i = 0; i < device_queues.size(); i++) {
		if (device_queues[i].queueFlags & vk::QueueFlagBits::eGraphics && device_queues[i].queueCount > gq_count + pq_count && m_physical_device.getSurfaceSupportKHR(i, m_window.get_surface())) {
			log << "Physical Device: using universal queue family: " << i ;
			gq_fam = i;
			pq_fam = i;
			break;
		}
		else if (device_queues[i].queueFlags & vk::QueueFlagBits::eGraphics && device_queues[i].queueCount > gq_count) {
			gq_fam = i;
			log << "Physical Device: using queue family for graphics: " << i ;
		}
		else if (m_physical_device.getSurfaceSupportKHR(i, m_window.get_surface()) && device_queues[i].queueCount > pq_count) {
			pq_fam = i;
			log << "Physical Device: using queue family for presentation: " << i ;
		}
	}

	if (gq_fam == std::numeric_limits<uint32_t>::max() || pq_fam == std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("Failed to find suitable queue(s).");
	}

	std::vector<vk::DeviceQueueCreateInfo> queue_infos;
	float q_priorities[] = { 1.0, 1.0 };

	if (gq_fam == pq_fam) {
		vk::DeviceQueueCreateInfo gq_info = { vk::DeviceQueueCreateFlags(), gq_fam, gq_count + pq_count, q_priorities };
		queue_infos.push_back(gq_info);

		//m_graphics_queue_info = queue_info(gq_fam, gq_count);
		m_graphics_queue_info.queue_family = gq_fam;
		m_graphics_queue_info.queue_indices = { 0 };
		log << "using gq fam: " << gq_fam << "using gq count: " << gq_count ;
		m_presentation_queue_info.queue_family = pq_fam;
		m_presentation_queue_info.queue_indices = { 1 };
		log << "using pq fam: " << pq_fam << "using pq count: " << pq_count ;
	}
	else {
		vk::DeviceQueueCreateInfo gq_info = { vk::DeviceQueueCreateFlags(), gq_fam, gq_count, q_priorities };
		queue_infos.push_back(gq_info);
		m_graphics_queue_info = queue_info(gq_fam, gq_count);

		vk::DeviceQueueCreateInfo pq_info = { vk::DeviceQueueCreateFlags(), pq_fam, pq_count, q_priorities };
		queue_infos.push_back(pq_info);
		m_graphics_queue_info = queue_info(pq_fam, pq_count);
	}

	//extensions
	std::vector<name_and_version> requiredExtensions = {
		{ "VK_KHR_swapchain", ANY_NAV_VERSION }
	};
	std::vector<const char*> checked_extensions;
    verify_device_extensions(requiredExtensions, checked_extensions);

	//create device
	vk::DeviceCreateInfo dev_info = { {}, static_cast<uint32_t>(queue_infos.size()), queue_infos.data(), 0, nullptr, static_cast<uint32_t>(checked_extensions.size()), checked_extensions.data(), nullptr };
	try {
		m_physical_device.createDevice(&dev_info, nullptr, &m_device);
	}
	catch (const std::exception &e) {
		log << "could not create extension: " << e.what()  ;
		throw std::runtime_error("could not create logical vulkan device");
	}

	//retrieve queue handles
	try {
		 m_graphics_queue = m_device.getQueue(gq_fam, m_graphics_queue_info.queue_indices[0]); 
		 m_presentation_queue = m_device.getQueue(gq_fam, m_presentation_queue_info.queue_indices[0]);
	}
	catch (const std::exception &e) {
		log << "could not retrieve vulkan queue handle(s): " << e.what()  ;
		throw std::runtime_error("could not retrieve vulkan queue handle(s)");
	}
}
void renderer::destroy_device()
{
	m_device.waitIdle();
	//destroy q's, phys + handle of dev
}

void renderer::verify_device_extensions(std::vector<name_and_version>& wanted, std::vector<const char*>& out)
{
	auto available = m_physical_device.enumerateDeviceExtensionProperties();
	vk::Bool32 found = 0;

	for (const auto& i : wanted) {
        found = 0;
		for (const auto a : available) {
			if (std::strcmp(a.extensionName, i.name) == 0 && i.version == ANY_NAV_VERSION || a.specVersion == i.version) {
				out.push_back(i.name);
				log << "Using device extension: " << a.extensionName ;
				found = 1;
				break;
			}
			else if (std::strcmp(a.extensionName, i.name) == 0) {
				out.push_back(i.name);
				log << "Warning: device extension version does not match" ;
				found = 1;
				break;
			}
		}
		if (!found) {
			log << "Missing device extension: " << i.name ;
			throw std::runtime_error("missing device extension");
		}
	}
}

//Render pass

vk::RenderPass renderer::create_render_pass(vk::Format format)
{
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

	vk::RenderPass holder;
	try {
		holder = m_device.createRenderPass(create_info, nullptr);
	}
	catch (const std::exception& e) {
		log << "Pipeline bot error: failed to create render pass: " << e.what() ;
	}

	log << "Pipeline bot: create render pass " << holder ;
	return holder;
}


void renderer::destroy_render_pass(vk::RenderPass handle)
{
		m_device.destroyRenderPass(handle, nullptr);
}


//Graphics pipeline

vk::Pipeline renderer::create_pipeline(vk::RenderPass& render_pass, vk::PipelineLayout& layout)
{
	//shader stages
	vk::ShaderModule vertex_module = create_shader(R"(C:\Users\Rory\Documents\projects\cw_game\resources/vert.spv)");
	vk::ShaderModule frag_module = create_shader(R"(C:\Users\Rory\Documents\projects\cw_game\resources/frag.spv)");
    //for temporary cleanup
    m_primary_shaders.push_back(vertex_module);
    m_primary_shaders.push_back(frag_module);
	
	vk::PipelineShaderStageCreateInfo vertex_stage_info = { {}, vk::ShaderStageFlagBits::eVertex, vertex_module, "main", {} };
	vk::PipelineShaderStageCreateInfo frag_stage_info = { {}, vk::ShaderStageFlagBits::eFragment, frag_module, "main", {} };

	vk::PipelineShaderStageCreateInfo shaders[] = { vertex_stage_info, frag_stage_info };

	//input TODO: add binding desc + attribute desc
	vk::PipelineVertexInputStateCreateInfo vertex_input_state = { {}, 0, {}, 0, {} };

	//input assembly TODO: set flast one to true for index buffers
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state = { {}, vk::PrimitiveTopology::eTriangleList, false };

	//viewport
	vk::Extent2D extent = m_window.get_image_extent();
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
		vk::FrontFace::eClockwise,
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
		{},																	// depth stencil
		&colour_blend_state,
		{},																	//TODO: implement dynamic state
		layout,
		render_pass
	};


	try {
		pipeline = m_device.createGraphicsPipeline(vk::PipelineCache(), create_info, nullptr);
	}
	catch (std::exception& e) {
		log << "failed to create graphics pipeline: " << e.what() ;
		throw;
	}
	log << "create graphics pipeline: " << pipeline ;
	return pipeline;
}

void renderer::destroy_pipeline(vk::Pipeline handle)
{
    //no need for WaitIdle() because that is already handled in the parent functions
    m_device.destroyPipeline(handle, nullptr);
    for(auto shader: m_primary_shaders) {
        m_device.destroyShaderModule(shader);
    }
}

//pipeline layout

vk::PipelineLayout renderer::create_pipeline_layout()
{
	//create
	vk::PipelineLayout layout;
	vk::PipelineLayoutCreateInfo layout_info = { {}, {}, {}, {}, {} };										//TODO: fill
	try {
		layout = m_device.createPipelineLayout(layout_info, nullptr);
	}
	catch (const std::exception& e) {
		log << "failed to create pipeline layout: " << e.what() ;
		throw std::runtime_error("failed to create pipeline layout");
	}
	log << "created pipeline layout " << layout ;
	return layout;
}

void renderer::destroy_pipeline_layout(vk::PipelineLayout handle)
{
		m_device.destroyPipelineLayout(handle, nullptr);
}

//helper

vk::ShaderModule renderer::create_shader(std::string path)
{
	std::ifstream file(path, std::ifstream::ate | std::ifstream::binary);
	if (!file.is_open()) {
		log << "failed to open shader file at " << path ;
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
		catch (const std::exception& e) {
			log << "failed to create shader module for: " << path ;
			log << e.what() ;
			throw std::runtime_error("failed to create shader module.");
		}
		return shader;
	}
}

//SEPERATOR: command buffers

void renderer::create_command_pool()
{
	vk::CommandPoolCreateInfo create_info = { {}, m_graphics_queue_info.queue_family };
	try {
		m_command_pool = m_device.createCommandPool(create_info);
	}
	catch (const std::exception& e) {
		log << "failed to create command pool: " << e.what() ;
		throw std::runtime_error("failed to create command pool.");
	}
	log << "created command pool.\n";
}

void renderer::destroy_command_pool()
{
	m_device.destroyCommandPool(m_command_pool);
}

vk::CommandBuffer renderer::create_command_buffer(vk::CommandBufferLevel level)
{
	vk::CommandBuffer out;
	vk::CommandBufferAllocateInfo info = { m_command_pool, level, 1 };
	try {
		m_device.allocateCommandBuffers(&info, &out);
	}
	catch (const std::exception& e) {
		log << "failed to allocate command buffer:" << e.what() ;
		throw std::runtime_error("failed to allocate command buffer.");
	}
	return out;
}

void renderer::destroy_command_buffer(vk::CommandBuffer buffer)
{
	m_device.freeCommandBuffers(m_command_pool, buffer);
}

void renderer::record_command_buffer(vk::CommandBuffer cmd_buffer, vk::Framebuffer framebuffer, vk::Pipeline pipeline, uint32_t vertex_count)
{
	vk::CommandBufferBeginInfo buf_info = { vk::CommandBufferUsageFlagBits::eSimultaneousUse, {} };
	try {
		cmd_buffer.begin(buf_info);
	}
	catch (const std::exception& e) {
		log << "failed to begin command buffer: " << e.what() ;
	}

	vk::Rect2D area = { {0, 0}, m_window.get_image_extent() };
	vk::ClearValue clear = vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
	vk::RenderPassBeginInfo rp_info = { m_primary_render_pass, framebuffer, area, 1, &clear };
	try {
		cmd_buffer.beginRenderPass(rp_info, vk::SubpassContents::eInline);								//TODO: modify for secondary command buffers
	}
	catch (std::exception& e) {
		log << "failed to begin render pass: " << e.what() ;
		throw std::runtime_error("see log.");
	}
	
	//draw
	cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	cmd_buffer.draw(vertex_count, 1, 0, 0);

	cmd_buffer.endRenderPass();

	try {
		cmd_buffer.end();
	}
	catch (const std::exception& e) {
		log << "Failed to record render pass:" << e.what() ;										//not fatal
	}
}

void renderer::create_drawing_enviroment()
{
	size_t count = m_window.m_framebuffers.size();
	log << "framebuffer size(): " << count ;
	m_command_buffers.resize(count);
	for (int i = 0; i < count; i++) {																	//create command buffers
		m_command_buffers[i] = create_command_buffer(vk::CommandBufferLevel::ePrimary);
		record_command_buffer(m_command_buffers[i], m_window.m_framebuffers[i], m_primary_pipeline, 3);			//TODO: make vertex count dynamic
		log << "created command buffer: " << m_command_buffers[i] ;
	}

	try {
		m_render_should_begin =	m_device.createSemaphore({});
		m_render_has_finished = m_device.createSemaphore({});
	}
	catch (const std::exception& e) {
		log << "failed to create synchronisation semaphores:" << e.what() ;
	}
}

void renderer::destroy_drawing_enviroment()
{
    m_device.waitIdle();            //safeguard
	m_device.destroySemaphore(m_render_should_begin);
	m_device.destroySemaphore(m_render_has_finished);

	for (auto item: m_command_buffers) {
		destroy_command_buffer(item);
	}
}

//draw command

void renderer::draw()
{
	//do logic here
	m_graphics_queue.waitIdle();
    vk::Result res;
    aquire:
        uint32_t img_index;
        vk::SwapchainKHR current_swapchain = m_window.get_swapchain();
        res = m_device.acquireNextImageKHR(current_swapchain, std::numeric_limits<uint64_t>::max(), m_render_should_begin, {}, &img_index);

        if(res == vk::Result::eErrorOutOfDateKHR) {
            destroy_drawing_enviroment();
            recreate_swapchain();
            recreate_pipeline();    //create a new render pass with the new extent and possible new format
            create_drawing_enviroment();
            goto aquire;
        }

    render:
        vk::Semaphore begin_sema[] = { m_render_should_begin };
        vk::Semaphore signal_sema[] = { m_render_has_finished };

        vk::PipelineStageFlags flags[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        vk::SubmitInfo submit_info = { 1, begin_sema, flags, 1, &m_command_buffers[img_index], 1, signal_sema };
        try {
            m_graphics_queue.submit(submit_info, {});
        }
        catch (const std::exception& e) {
            log << "failed to submit command buffer to the graphics queue: " << e.what() ;
        }

	present:
        vk::PresentInfoKHR pres_info = { 1, signal_sema, 1, &current_swapchain, &img_index, {} };
        try {
            res = m_presentation_queue.presentKHR(pres_info);
        }
        catch (const std::exception& e) {
            log << "failed to present: " << e.what() ;
        }



}

void renderer::create_swapchain()
{
	m_window.set_device(m_device);
	m_window.set_physical_device(m_physical_device);
	m_window.set_presentation_queue(m_presentation_queue);
	m_window.set_presentation_queue_info(&m_presentation_queue_info);
	m_window.create_swapchain();
	m_window.create_image_views();
}

void renderer::clear_swapchain()
{
    m_device.waitIdle();
	m_window.destroy_swapchain();
}

void renderer::recreate_swapchain()
{
	clear_swapchain();
	create_swapchain();
}

void renderer::create_pipeline()
{
    log << "creating pipeline...";
    m_primary_render_pass = create_render_pass(m_window.get_image_format());
    m_primary_layout = create_pipeline_layout();
    m_primary_pipeline = create_pipeline(m_primary_render_pass, m_primary_layout);
    m_window.create_framebuffers(m_primary_render_pass);
}

void renderer::clear_pipeline()
{
    m_device.waitIdle();
    log << "clearing pipeline...";
    m_window.destroy_framebuffers();
    destroy_render_pass(m_primary_render_pass);
    destroy_pipeline_layout(m_primary_layout);
    destroy_pipeline(m_primary_pipeline);
}

void renderer::recreate_pipeline()
{
    clear_pipeline();
    create_pipeline();
}


}