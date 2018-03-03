#include "index_buffer.h"

namespace cwg {
namespace graphics {

index_buffer::index_buffer(vk::Device dev, vk::PhysicalDevice p_dev, vk::DeviceSize total_size) :
    buffer_base(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal),
    log("index_buffer", "log/ib.log", {})
{
    m_device = dev;
    m_total_size = total_size;

    create(m_total_size);
    allocate(p_dev);
}

index_buffer::~index_buffer()
{
    deallocate();
    destroy();
}

}
}