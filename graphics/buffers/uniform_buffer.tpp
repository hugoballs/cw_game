

void write(void *src, size_t size)
{
    assert(size <= m_total_size);
    void *cpu_mem = m_device.mapMemory(m_device_memory, 0, m_total_size, {});
    memcpy(cpu_mem, src, size);
    m_device.unmapMemory(m_device_memory);
}