//#include "vk_functions.h"		//need to build into the vulkan renderer
#include "logger.h"
#include "graphics/renderer.h"
#include "logic/logic.h"

#include <thread>

/* "Make a game, not an engine." ~ someone from a game engine lecture on youtube*/
/* my thoughts: I've never made a game, so how could I possibly know what a game engine should be .*/

int main() {
	cwg::logger l("main", "log/main.log", {});

	cwg::renderer render;
	//cwg::logic::init();
	while (!render.should_close()) {
		render.draw();
		render.poll_events();
		//TODO: commit to github
	}

	return 0;
}
