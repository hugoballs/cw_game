#include "staging_buffer.h"

namespace cwg {
namespace graphics {

staging_buffer::staging_buffer(vk::Device dev, vk::PhysicalDevice p_dev, std::vector<float>& data, vk::DeviceSize total_size, vk::DeviceSize vertex_size) :
    buffer_base(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
{
    m_device = dev;
    m_total_size = total_size;
    m_vertex_size = vertex_size;

    create(m_total_size);
    allocate(p_dev);
    map(data, m_total_size);
}

staging_buffer::~staging_buffer()
{
    deallocate();
    destroy();
}

void staging_buffer::map(std::vector<float>& data, vk::DeviceSize size)
{
    void *cpu_mem = m_device.mapMemory(buffer_base::m_device_memory, 0, size, {});
    memcpy(cpu_mem, data.data(), static_cast<size_t>(size));
    m_device.unmapMemory(buffer_base::m_device_memory);
}

void staging_buffer::copy(vertex_buffer& dst, vk::CommandPool pool, vk::Queue queue, vk::DeviceSize src_offset, vk::DeviceSize dst_offset)
{
    vk::CommandBufferAllocateInfo alloc_info = { pool, vk::CommandBufferLevel::ePrimary, 1};
    vk::CommandBuffer cmd_buffer;
    try {
        m_device.allocateCommandBuffers(&alloc_info, &cmd_buffer);
    }
    catch(...) {
        throw std::runtime_error("error failed to allocate transfer command buffer for staging buffer.");
    }

    vk::CommandBufferBeginInfo buf_begin = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit, {} };
    cmd_buffer.begin(buf_begin);

    vk::BufferCopy region_info = { src_offset, dst_offset, m_total_size };
    cmd_buffer.copyBuffer(m_handle, dst.get(), region_info);
    cmd_buffer.end();

    vk::SubmitInfo submit_info = { {}, {}, {}, 1, &cmd_buffer, {}, {}};
    vk::Fence wait_fence = m_device.createFence( {} );                    //fence is more scalable for multiple transfers
    queue.submit( {submit_info}, wait_fence );

    m_device.waitForFences( { wait_fence }, true, std::numeric_limits<uint64_t>::max());
    m_device.freeCommandBuffers(pool, {cmd_buffer} );
    m_device.destroyFence(wait_fence);

    dst.set_total_size(m_total_size);
    dst.set_vertex_size(m_vertex_size);
}

}
}