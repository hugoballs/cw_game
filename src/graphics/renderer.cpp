#include <cstring>
#include <fstream>
#include <array>
#include <chrono>

#include "renderer.h"
#include "buffers/uniform_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../dependencies/stb_image.h"

//glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
	//NOTE: IMPORTANT: make sure the vertices are in the correct order
	//note: depth-buffering is required for the difference to become perceivable.
	std::vector<float> data = {
		-0.5, -0.5, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
		0.5, -0.5, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0,
		-0.5, 0.5, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0,

		-0.5, -0.5, -0.5, 1.0, 0.0, 0.0, 0.0, 0.0,
		0.5, -0.5, -0.5, 0.0, 1.0, 0.0, 1.0, 0.0,
		0.5, 0.5, -0.5, 0.0, 0.0, 1.0, 1.0, 1.0,
		-0.5, 0.5, -0.5, 1.0, 1.0, 1.0, 0.0, 1.0
	};
	auto t_size = data.size() * sizeof(float);
	auto v_size = (3 + 3 + 2) * sizeof(float);

	m_staging_buffer.reset(m_device, m_physical_device, data, t_size, v_size);
	m_primary_vb.reset(m_device, m_physical_device, t_size, v_size);
	m_staging_buffer.copy(m_primary_vb, m_transfer_pool, m_graphics_queue);
	m_staging_buffer.reset();

	m_primary_vb.set_attribute(0, 0, 3);	//position
	m_primary_vb.set_attribute(0, 1, 3);	//colour
	m_primary_vb.set_attribute(0, 2, 2);	//texture coords

	std::vector<uint32_t> indices = {
		0, 1, 2,
		2, 3, 0,
		4, 5, 6,
		6, 7, 4
	};

	m_staging_buffer.reset(m_device, m_physical_device, indices, indices.size() * sizeof(uint32_t));
	m_primary_ib.reset(m_device, m_physical_device, indices.size() * sizeof(uint32_t));
	m_staging_buffer.copy(m_primary_ib, m_transfer_pool, m_graphics_queue);
	m_staging_buffer.reset();

	m_uniform_buffer.reset(m_device, m_physical_device, m_uniform_buffer_size);
	
	create_texture("resources/sculpture.jpg");

	create_descriptor_pool(1);
	create_descriptor_set_layout();
	create_descriptor_set();
	update_uniform_buffer();

	create_pipeline();
	create_drawing_enviroment(m_primary_vb);
}

renderer::~renderer()
{
	log << "last recorded fps: " << m_fps_counter.get_last();
	m_device.waitIdle();
	m_staging_buffer.reset();
	destroy_texture();
	m_uniform_buffer.reset();
	destroy_descriptor_set();
	destroy_descriptor_set_layout();
	destroy_descriptor_pool();
	m_primary_ib.reset();
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
	//TODO: detect XLIB vs XCB
	instance_extensions.push_back({ "VK_KHR_xcb_surface", ANY_NAV_VERSION });
	instance_extensions.push_back({ "VK_KHR_xlib_surface", ANY_NAV_VERSION });
#endif

		std::vector<name_and_version> instance_layers;
#ifndef NDEBUG
		instance_layers.push_back( { "VK_LAYER_LUNARG_standard_validation", ANY_NAV_VERSION } );
		//instance_layers.push_back( {"VK_LAYER_LUNARG_assistant_layer", ANY_NAV_VERSION } );
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
			m_sampler_anistropy = device.getFeatures().samplerAnisotropy;
			break;
		}
	}

	//queue families
	std::vector<vk::QueueFamilyProperties> device_queues = m_physical_device.getQueueFamilyProperties();
	uint32_t gq_count = 1;
	uint32_t pq_count = 1;
	uint32_t gq_fam = std::numeric_limits<uint32_t>::max();
	uint32_t pq_fam = std::numeric_limits<uint32_t>::max();

	//TODO: eventually make better selection algorithm for queues
	for (unsigned int i = 0; i < device_queues.size(); i++) {
		if (device_queues[i].queueFlags & vk::QueueFlagBits::eGraphics && device_queues[i].queueCount >= 1 && m_physical_device.getSurfaceSupportKHR(i, m_window.get_surface())) {
			log << "Physical Device: using universal queue family: " << i ;
			gq_fam = i;
			pq_fam = i;
			break;
		}
	}

	if (gq_fam == std::numeric_limits<uint32_t>::max() || pq_fam == std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("Failed to find suitable queue(s).");
	}

	const float priorites[] = { 1.0 };
	vk::DeviceQueueCreateInfo queue_info = { {}, gq_fam, gq_count, priorites };
	vk::DeviceQueueCreateInfo queues[] = { queue_info };
	m_graphics_queue_info.queue_family = gq_fam;
	m_graphics_queue_info.queue_indices = { 0 };
	m_presentation_queue_info = m_graphics_queue_info;

	//extensions
	std::vector<name_and_version> requiredExtensions = {
		{ "VK_KHR_swapchain", ANY_NAV_VERSION }
	};
	std::vector<const char*> checked_extensions;
    verify_device_extensions(requiredExtensions, checked_extensions);

	//device features
	vk::PhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = true;
	//create device
	vk::DeviceCreateInfo dev_info = { {}, 1, queues, 0, nullptr, static_cast<uint32_t>(checked_extensions.size()), checked_extensions.data(), &features };
	
	try {
		m_physical_device.createDevice(&dev_info, nullptr, &m_device);
		log << "created device";
	}
	catch (const std::exception &e) {
		log << "could not create extension: " << e.what()  ;
		throw std::runtime_error("could not create logical vulkan device");
	}


	//retrieve queue handles
	try {
		 m_graphics_queue = m_device.getQueue(gq_fam, m_graphics_queue_info.queue_indices[0]);
		 m_presentation_queue = m_graphics_queue;		//atm only using 1 queue
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

void renderer::create_descriptor_pool(uint32_t max_sets)
{
	std::array<vk::DescriptorPoolSize, 2> sizes;
	sizes[0] = { vk::DescriptorType::eUniformBuffer, 1 };
	sizes[1] = { vk::DescriptorType::eCombinedImageSampler, 1 };
	vk::DescriptorPoolCreateInfo pool_info = { {} ,max_sets, static_cast<uint32_t>(sizes.size()), sizes.data() };
	try {
		m_descriptor_pool = m_device.createDescriptorPool(pool_info);
	}
	catch (const std::exception& e) {
		log << "failed to create command pool: " << e.what() ;
		throw std::runtime_error("failed to create command pool.");
	}
	log << "created descriptor pool.\n";
}

void renderer::destroy_descriptor_pool()
{
	m_device.destroyDescriptorPool(m_descriptor_pool);
}

//Descriptor Sets

void renderer::create_descriptor_set_layout()
{
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings;
	bindings[0] = { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, {} };	//ubo
	bindings[1] = { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, {} };	//sampler

    vk::DescriptorSetLayoutCreateInfo ci = { {}, static_cast<uint32_t>(bindings.size()), bindings.data() };
    vk::DescriptorSetLayout layout;
    try {
		m_descriptor_layout = m_device.createDescriptorSetLayout(ci);
    }
    catch(const std::exception& e) {
		throw std::runtime_error("failed to create descriptor set layout.");
    }
}

void renderer::destroy_descriptor_set_layout()
{
	m_device.destroyDescriptorSetLayout(m_descriptor_layout);
}

void renderer::create_descriptor_set()
{
	vk::DescriptorSetAllocateInfo alloc = { m_descriptor_pool, 1, &m_descriptor_layout };
	try {
		std::vector<vk::DescriptorSet> sets = m_device.allocateDescriptorSets(alloc);
		if(sets.size() > 1 ) {
			throw std::runtime_error("invalid number of descriptor sets.");
		}
		m_descriptor_set = sets.front();
	}
	catch(const std::exception& e) {
		throw;
	}

	//configure
	vk::DescriptorBufferInfo buf_info = { m_uniform_buffer.get(), 0, m_uniform_buffer_size };
	vk::DescriptorImageInfo img_info = { m_tex_sampler, m_tex_view, vk::ImageLayout::eShaderReadOnlyOptimal };
	std::array<vk::WriteDescriptorSet, 2> write_info;

	write_info[0] = { m_descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &buf_info, {} };
	write_info[1] = { m_descriptor_set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &img_info, {}, {} };
	try {
		m_device.updateDescriptorSets(write_info, {});
	}
	catch(const std::exception& e) {
		throw std::runtime_error("failed to update descriptor set.");
	}
}

void renderer::destroy_descriptor_set()
{
	//no need, will be destroyed along with pool
}

void renderer::update_uniform_buffer()
{
	struct ubo {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	static auto t1 = std::chrono::steady_clock::now();
	auto t2 = std::chrono::steady_clock::now();
	float delta = std::chrono::duration<float, std::chrono::seconds::period>(t2 - t1).count();

	ubo this_obj_ubo;
	this_obj_ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//this_obj_ubo.model = glm::mat4(1.0f);
	//this_obj_ubo.model = glm::rotate(this_obj_ubo.model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//NOTE: IMPORTANT! the up vector is defined as the z-axis
	this_obj_ubo.view = glm::lookAt(glm::vec3(-0.25f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	vk::Extent2D e = m_window.get_image_extent();
	this_obj_ubo.proj = glm::perspective(glm::radians(90.0f), float(e.width) / float(e.height), 0.1f, 10.0f);

	//NOTE: this is required for vulkan's inverted coordinate system
	//it's also probably part of why my own projection matrix didn't work.
	this_obj_ubo.proj[1][1] *= -1;

	m_uniform_buffer.write(&this_obj_ubo, m_uniform_buffer_size);
}


//Images

void renderer::create_texture(std::string path)
{
	int32_t width, height, nchannels;
	unsigned char *img = stbi_load(path.c_str(), &width, &height, &nchannels, STBI_rgb_alpha);
	vk::DeviceSize size = width * height * 4;
	log << "image size is: " << size;
	if(!img) {
		throw std::runtime_error("failed to stbi_load()");
	}

	create_image(&m_tex, &m_tex_mem, width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);
	m_staging_buffer.reset(m_device, m_physical_device, img, size);
	transition_image_layout(m_tex, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	m_staging_buffer.copy(m_tex, m_transfer_pool, m_graphics_queue, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	transition_image_layout(m_tex, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	create_image_view(&m_tex, &m_tex_view, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
	create_sampler();

	stbi_image_free(img);
}

void renderer::destroy_texture()
{
	destroy_sampler();
	destroy_image_view(&m_tex_view);
	destroy_image(&m_tex, &m_tex_mem);
}

void renderer::create_image(vk::Image *img, vk::DeviceMemory *mem, int32_t width, int32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits mem_flags)
{
	
	//create image
	vk::ImageCreateInfo ci = {
		{},
		vk::ImageType::e2D,
		format,
		{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
		1,
		1,
		vk::SampleCountFlagBits::e1,
		tiling,
		usage,
		vk::SharingMode::eExclusive,
		{},
		{},
		vk::ImageLayout::eUndefined
	};

	//allocate memory
	try {
		*img = m_device.createImage(ci);
	}
	catch(const std::exception& e) {
		throw std::runtime_error("failed to create vk::Image");
	}

	vk::MemoryRequirements mem_req = m_device.getImageMemoryRequirements(*img);
	vk::PhysicalDeviceMemoryProperties p_props = m_physical_device.getMemoryProperties();

	uint32_t mem_i = std::numeric_limits<uint32_t>::max();
	
	for(uint32_t i = 0; i < p_props.memoryTypeCount; i++) {
		if(mem_req.memoryTypeBits & (1 << i) && p_props.memoryTypes[i].propertyFlags & (mem_flags)) {
			mem_i = i;
			break;
		}
	}
	
	if(mem_i == std::numeric_limits<uint32_t>::max()) {
		throw std::runtime_error("failed to find suitable device memory.");
	}

	vk::MemoryAllocateInfo ai = { mem_req.size, mem_i };

	try {
		*mem = m_device.allocateMemory(ai);
	}
	catch(const std::exception& e) {
		throw std::runtime_error("failed to allocate memory");
	}

	m_device.bindImageMemory(*img, *mem, 0);

	log << "created + allocated image";
}

void renderer::destroy_image(vk::Image *img, vk::DeviceMemory *img_mem)
{
	m_device.freeMemory(*img_mem);
	m_device.destroyImage(*img);
}

void renderer::transition_image_layout(vk::Image& img, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
	log << "begin transition";
	vk::CommandBuffer cmd_buffer = create_command_buffer(vk::CommandBufferLevel::ePrimary);
	vk::CommandBufferBeginInfo bi = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {} };
	cmd_buffer.begin(bi);
	/* commands here */
	vk::AccessFlags src_access_flags;
	vk::AccessFlags dst_access_flags;
	vk::PipelineStageFlags src_flags;
	vk::PipelineStageFlags dst_flags;
	vk::ImageAspectFlags asp_flags;

	//determine the access rules
	if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
		src_access_flags = vk::AccessFlags();
		dst_access_flags = vk::AccessFlagBits::eTransferWrite;
		src_flags = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_flags = vk::PipelineStageFlagBits::eTransfer;
	}
	else if(old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		src_access_flags = vk::AccessFlagBits::eTransferWrite;
		dst_access_flags = vk::AccessFlagBits::eShaderRead;
		src_flags = vk::PipelineStageFlagBits::eTransfer;
		dst_flags = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		src_access_flags = vk::AccessFlags();
		dst_access_flags = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		src_flags = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_flags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else {
		throw std::runtime_error("invalid access rules for transisitoning image layout!");
	}

	//determine the aspect flags
	if(new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		asp_flags = vk::ImageAspectFlagBits::eDepth;

		if(format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint) {
			asp_flags |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else {
		asp_flags = vk::ImageAspectFlagBits::eColor;
	}
	
	vk::ImageMemoryBarrier barrier = {
		src_access_flags,
		dst_access_flags,
		old_layout,
		new_layout,
		{},
		{},
		img,
		{ asp_flags , 0, 1, 0, 1 }
	};
	cmd_buffer.pipelineBarrier(
		src_flags,
		dst_flags,
		vk::DependencyFlagBits::eByRegion,
		{},
		{},
		{ barrier }
	);
	/* end commands */
	cmd_buffer.end();
	vk::Fence end_fence = m_device.createFence({});
	vk::SubmitInfo si = { {}, {}, {}, 1, &cmd_buffer, {}, {} };
	m_graphics_queue.submit({ si}, end_fence);

	m_device.waitForFences({ end_fence }, true, std::numeric_limits<uint64_t>::max());
	m_device.freeCommandBuffers(m_command_pool, {cmd_buffer});
	m_device.destroyFence(end_fence);
	log << "end transition.";
}

void renderer::create_image_view(vk::Image *img, vk::ImageView *iv, vk::Format format, vk::ImageAspectFlagBits asp_flags)
{
	vk::ComponentMapping components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity , vk::ComponentSwizzle::eIdentity };
	vk::ImageSubresourceRange subrange = { asp_flags , 0, 1, 0, 1 };
	vk::ImageViewCreateInfo ci = { {}, *img, vk::ImageViewType::e2D, format, components, subrange };
	try {
		*iv = m_device.createImageView(ci);
	}
	catch(const std::exception& e) {
		throw std::runtime_error("failed to create image view.");
	}
}

void renderer::destroy_image_view(vk::ImageView *iv)
{
	m_device.destroyImageView(*iv);
}

void renderer::create_sampler()
{
	vk::SamplerCreateInfo ci = {
		{},
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		0.0f,
		m_sampler_anistropy,
		16.0f,
		false,
		vk::CompareOp::eAlways,
		0.0f,
		0.0f,
		vk::BorderColor::eIntOpaqueBlack,
		false
	};
	try {
		m_tex_sampler = m_device.createSampler(ci);
	}
	catch(const std::exception& e) {
		throw std::runtime_error("failed to create sampler.");
	}
}

void renderer::destroy_sampler()
{
	m_device.destroySampler(m_tex_sampler);
}

//depth buffer

void renderer::create_depth_buffer()
{
	m_depth_format = select_image_format(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	vk::Extent2D e = m_window.get_image_extent();
	create_image(&m_depth_image, &m_depth_mem, e.width, e.height, m_depth_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
	create_image_view(&m_depth_image, &m_depth_view, m_depth_format, vk::ImageAspectFlagBits::eDepth);
	transition_image_layout(m_depth_image, m_depth_format, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void renderer::destroy_depth_buffer() {
	destroy_image_view(&m_depth_view);
	destroy_image(&m_depth_image, &m_depth_mem);
}

vk::Format renderer::select_image_format(std::vector<vk::Format>&& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	if(tiling == vk::ImageTiling::eLinear) {
		for(const auto& format : formats) {
			vk::FormatProperties props = m_physical_device.getFormatProperties(format);
			if(props.linearTilingFeatures & features) {
				return format;
			}
		}
		//error
		throw std::runtime_error("could not find linear tiling format with requested features.");
	}
	else if(tiling == vk::ImageTiling::eOptimal) {
		for(const auto& format : formats) {
			vk::FormatProperties props = m_physical_device.getFormatProperties(format);
			if(props.optimalTilingFeatures & features) {
				return format;
			}
		}
		throw std::runtime_error("could not find optimal tiling format with requested features.");
	}
}

//Command buffers

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
	std::array<vk::ClearValue, 2> clear =  {
		vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }),
		vk::ClearDepthStencilValue(1.0f, 0)
	};
	vk::RenderPassBeginInfo rp_info = { m_primary_render_pass.get(), framebuffer, area, static_cast<uint32_t>(clear.size()), clear.data() };
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
	cmd_buffer.bindIndexBuffer(m_primary_ib.get(), 0, m_primary_ib.get_index_type());
	cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_primary_layout.get(), 0, { m_descriptor_set }, {});
	
	
	cmd_buffer.drawIndexed(m_primary_ib.size(), 1, 0, 0, 0);
	//cmd_buffer.draw(vb.size(), 1, 0, 0);

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
	//update_uniform_buffer();
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
	create_depth_buffer();
    m_primary_render_pass.reset(m_device, m_window.get_image_format(), m_depth_format);
	//m_descriptor_layouts.clear();
	//m_descriptor_layouts.push_back(m_descriptor_set.get_layout());
    m_primary_layout.reset(m_device, &m_descriptor_layout);
    m_primary_pipeline.reset(m_device, m_primary_render_pass.get(), m_primary_layout.get(), m_window.get_image_extent(), &m_primary_vb);
    m_window.create_framebuffers(m_primary_render_pass.get(), m_depth_view);
}

void renderer::clear_pipeline()
{
    m_device.waitIdle();
    log << "clearing pipeline...";
	destroy_depth_buffer();
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
