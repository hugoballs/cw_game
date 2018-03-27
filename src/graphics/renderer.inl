#ifndef RENDERER_INL
#define RENDERER_INL
#include <vector>
#include <iostream>

/* This file just contains a few extra little structs that aren't worth their own file */

namespace cwg {
#define ANY_NAV_VERSION std::numeric_limits<uint32_t>::max()
	struct name_and_version {
		const char *name;
		uint32_t version;
	};

	struct queue_info {
		uint32_t queue_family;
		std::vector<uint32_t> queue_indices;
		inline queue_info() {};
		inline uint32_t get_count() { return (uint32_t)queue_indices.size(); }
		inline void set_num_indices(uint32_t count) { for (uint32_t i = 0; i < count; i++) { queue_indices.push_back(i); } }
		inline void set_num_indices(uint32_t count, uint32_t offset) { for (uint32_t i = 0; i < count; i++) { queue_indices.push_back(i + offset); } }
		inline queue_info(uint32_t q_fam, uint32_t num_indices) : queue_family(q_fam) { this->set_num_indices(num_indices); }
		inline queue_info(uint32_t q_fam, uint32_t num_indices, uint32_t offset) : queue_family(q_fam) { this->set_num_indices(num_indices, offset); }
	};
}

#endif