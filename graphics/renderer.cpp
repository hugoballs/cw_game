#include <cstring>
#include <fstream>

#include "renderer.h"

namespace cwg {
namespace graphics {

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
	create_transfer_pool();
	//caution: vulkan uses inverted y axis
	std::vector<float> data = {
		0.0, -0.5, 1.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0, 0.0,
		-0.5, 0.5, 0.0, 0.0, 1.0
	};
	auto t_size = data.size() * sizeof(float);
	auto v_size = 5 * sizeof(float);

	m_staging_buffer.reset(m_device, m_physical_device, data, t_size, v_size);
	m_primary_vb.reset(m_device, m_physical_device, t_size, v_size);
	m_staging_buffer.copy(m_primary_vb, m_transfer_pool, m_graphics_queue);
	m_staging_buffer.reset();

	m_primary_vb.set_attribute(0, 0, 2);
	m_primary_vb.set_attribute(0, 1, 3);

	create_pipeline();
	create_drawing_enviroment(m_primary_vb);
}

renderer::~renderer()
{
	log << "last recorded fps: " << m_fps_counter.get_last();
	m_device.waitIdle();
	m_primary_vb.reset();
    destroy_drawing_enviroment();
	clear_pipeline();
	destroy_transfer_pool();
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
		instance_layers.push_back( { "VK_LAYER_LUNARG_standard_validation", ANY_NAV_VERSION } );
		instance_layers.push_back( {"VK_LAYER_LUNARG_assistant_layer", ANY_NAV_VERSION });
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
			log << "Physical Device: using " << device.getProperties().deviceName;
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

void renderer::create_transfer_pool()
{
	vk::CommandPoolCreateInfo create_info = { vk::CommandPoolCreateFlagBits::eTransient , m_graphics_queue_info.queue_family };
	try {
		m_transfer_pool = m_device.createCommandPool(create_info);
	}
	catch (const std::exception& e) {
		log << "failed to create command pool: " << e.what() ;
		throw std::runtime_error("failed to create command pool.");
	}
	log << "created command pool.\n";
}

void renderer::destroy_transfer_pool()
{
	m_device.destroyCommandPool(m_transfer_pool);
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

void renderer::record_command_buffer(vk::CommandBuffer cmd_buffer, vk::Framebuffer framebuffer, vk::Pipeline pipeline, graphics::vertex_buffer& vb)
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
	vk::RenderPassBeginInfo rp_info = { m_primary_render_pass.get(), framebuffer, area, 1, &clear };
	try {
		cmd_buffer.beginRenderPass(rp_info, vk::SubpassContents::eInline);								//TODO: modify for secondary command buffers
	}
	catch (std::exception& e) {
		log << "failed to begin render pass: " << e.what() ;
		throw std::runtime_error("see log.");
	}
	
	//draw
	cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	cmd_buffer.bindVertexBuffers(0, { vb.get() }, { 0 });
	log << "vb size(): " << vb.size();
	cmd_buffer.draw(vb.size(), 1, 0, 0);

	cmd_buffer.endRenderPass();

	try {
		cmd_buffer.end();
	}
	catch (const std::exception& e) {
		log << "Failed to record render pass:" << e.what() ;										//not fatal
	}
}

void renderer::create_drawing_enviroment(graphics::vertex_buffer& vb)
{
	size_t count = m_window.m_framebuffers.size();
	log << "framebuffer size(): " << count ;
	m_command_buffers.resize(count);
	for (int i = 0; i < count; i++) {																	//create command buffers
		m_command_buffers[i] = create_command_buffer(vk::CommandBufferLevel::ePrimary);
		record_command_buffer(m_command_buffers[i], m_window.m_framebuffers[i], m_primary_pipeline.get(), vb);			//TODO: make vertex count dynamic
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
	m_fps_counter.tick(std::chrono::steady_clock::now());
	
    vk::Result res;
    aquire:
        uint32_t img_index;
        vk::SwapchainKHR current_swapchain = m_window.get_swapchain();
        res = m_device.acquireNextImageKHR(current_swapchain, std::numeric_limits<uint64_t>::max(), m_render_should_begin, {}, &img_index);

        if(res == vk::Result::eErrorOutOfDateKHR) {
            destroy_drawing_enviroment();
            recreate_swapchain();
            recreate_pipeline();    //create a new render pass with the new extent and possible new format
            create_drawing_enviroment(m_primary_vb);
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
    m_primary_render_pass.reset(m_device, m_window.get_image_format());
    m_primary_layout.reset(m_device);
    m_primary_pipeline.reset(m_device, m_primary_render_pass.get(), m_primary_layout.get(), m_window.get_image_extent(), &m_primary_vb);
    m_window.create_framebuffers(m_primary_render_pass.get());
}

void renderer::clear_pipeline()
{
    m_device.waitIdle();
    log << "clearing pipeline...";
    m_window.destroy_framebuffers();
    m_primary_render_pass.reset();
    m_primary_layout.reset();
	m_primary_pipeline.reset();
}

void renderer::recreate_pipeline()
{
    clear_pipeline();
    create_pipeline();
}

}
}