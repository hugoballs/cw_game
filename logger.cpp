#include "logger.h"


namespace cwg {

logger::logger(const std::string src, logger_flags flags) : m_src_str(src)
{
	m_src_str.shrink_to_fit();									//free unused memory as this cannot be changed
	//validate flags
	m_flags |= flags;
}

logger::logger(const std::string src, const std::string log_path, logger_flags flags) : m_src_str(src), m_file_path(log_path)
{
	m_src_str.shrink_to_fit();									//free memory
	m_file_path.shrink_to_fit();
	//validate flags
	m_flags |= logger_flags::file_too;							//unconditionally in this constructor
	m_flags |= flags;
}

logger::~logger()
{

}

}