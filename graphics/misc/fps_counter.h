#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <mutex>
#include <chrono>

namespace cwg {
namespace graphics {

class fps_counter {
    uint32_t m_last_fps;
    std::mutex m_fps_mu;

public:
    fps_counter() {}
    ~fps_counter() {}

    void tick(std::chrono::steady_clock::time_point tp);
    inline uint32_t get_last() { return m_last_fps; }
};

}
}
#endif