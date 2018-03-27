#include "fps_counter.h"

#include <queue>

namespace cwg {
namespace graphics {

void fps_counter::tick(std::chrono::steady_clock::time_point tp)
{
    static uint32_t m_frame_count = 0;
    static std::queue<uint32_t> avg_frames;
	static std::chrono::steady_clock::time_point last_tp;
	static bool init = false;
	if(!init) { last_tp = tp; init = true; }
	
	if(std::chrono::duration_cast<std::chrono::milliseconds>(tp - last_tp) >= std::chrono::milliseconds(250)) {
		avg_frames.push(m_frame_count);
		m_frame_count = 0;
		last_tp = tp;
	}
	else {
		m_frame_count++;
	}
	
	if(avg_frames.size() >= 4) {
		uint32_t frames = 0;
		uint32_t s = static_cast<uint32_t>(avg_frames.size());
		for(uint32_t i = 0; i < s; i++) { frames += avg_frames.front(); avg_frames.pop(); }

        std::lock_guard<std::mutex> lock(m_fps_mu);
		m_last_fps = frames;		//uses the average frames in a 1/4 second to obtain a more realistic number on average
	}
}

}
}
