#ifndef LOGGER_H
#define LOGGER_H

/*
Usage: construct a logger object and call either ouput or use operator<< to log. see flags for options
*/

#include <iostream>
#include <mutex>
#include <fstream>
#include <ctime>
#include <string>
//#include <typeinfo>                   //caution: if the header is not included, every use of the keyword typeid makes the program ill-formed.

namespace cwg {

enum class line {
	endl
};

enum logger_flags {
	file_too = 0b0000'0001,
	file_only = 0b0000'0010,
	no_colour = 0b0000'0100
};

class logger {
private:
	std::string m_src_str;
	std::string m_file_path;
	unsigned char m_flags;																
	std::ofstream m_file;
	std::once_flag m_file_once_flag;
	std::mutex m_file_mu;

	template<typename T>
	void log_to_screen(T a, char *time)                                                     //usage: the dev must specify a new line
	{
		//process
		static std::mutex m_cout_mu; //shared across all logger instances
        //if(typeid(a) != typeid(cwg::line)) {
            std::lock_guard<std::mutex> lock(m_cout_mu);
            std::cout << "[" << std::string(time) << "][" << m_src_str << "]: " << a << std::endl;
        //}
	}

	/*template <>
	void log_to_screen<enum class line>(line a, char *time)
	{
		if(a == line::endl) {
			std::cout << "\n";
		}
	}*/


	template<typename T>
	void log_to_file(T a, char *time)
	{
		//process
		std::call_once(m_file_once_flag, [this]() { m_file.open(m_file_path); });		//lazy init
		//note: pretty nasty bug: call_once throws std::system_error if not linked to -pthread
		std::lock_guard<std::mutex> lock(m_file_mu);
		m_file << "[" << time << "][" << m_src_str << "]: " << a << std::endl;
	}

    /*template<>
    void log_to_file<enum class line>(line a, char *time)
    {
        //do nothing
    }*/

public:
	logger(const std::string src, logger_flags flags);									//most basic constructor
	logger(const std::string src, const std::string log_path, logger_flags flags);		//log to file too
	//logger(const logger& obj) = delete;
    void operator=(const logger obj) = delete;
	~logger();

	
	template<typename T>
	void output(T a)
	{
		std::time_t t = std::time(nullptr);
#ifdef _WIN32
		std::tm time;
		localtime_s(&time, &t);
#else
		std::tm time = *std::localtime(&t);
#endif
		char t_str[20];
		std::strftime(t_str, sizeof(t_str), "%T", &time);
#ifndef NDEBUG                                                                          //dont print anything to screen in release
		log_to_screen(a, t_str);
#endif
		if(m_flags & logger_flags::file_too) {
			log_to_file(a, t_str);
		}
	}

	template<typename T>
	friend logger& operator<<(logger& log, T a)
	{
		log.output(a);
		return log;
	}

	/*friend logger& operator<<(logger& log, line a)
	{
		log.output(a);
		return log;
	}*/

};

}
#endif