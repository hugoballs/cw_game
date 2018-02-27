#include "logic.h"

#include "../logger.h"
#include <memory>

namespace cwg {
namespace logic {

namespace {								//anonymous namespace
	logger log("logic_master", "log/logic.log", {});
	//std::unique_ptr<std::forward_list<logic_base>> logic_stack;
}

void init()
{
	log << "logic initialising...";
}

}
}
