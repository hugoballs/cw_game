//#include "vk_functions.h"		//need to build into the vulkan renderer
#include "logger.h"
#include "graphics/renderer.h"
#include "logic/logic.h"

#include "maths/vec.h"
#include "maths/mat.h"

#include <iostream>
#include <chrono>

/* "Make a game, not an engine." ~ someone from a game engine lecture on youtube*/
/* my thoughts: I've never made a game, so how could I possibly know what a game engine should be .*/

int main(int argc, char **argv) {
	cwg::logger l("main", "log/main.log", {});

	if(argc < 2) { l << "no args."; }

	auto t1 = std::chrono::steady_clock::now();

	cwg::graphics::renderer render;
	auto t2 = std::chrono::steady_clock::now();
	std::chrono::duration<double, std::milli> t_elapsed = t2 - t1;
	std::cout << "Took: " << t_elapsed.count() << "ms" << std::endl;
	//cwg::logic::init();
	while (!render.should_close()) {
		render.draw();
		render.poll_events();
	}
	
	return 0;
}
