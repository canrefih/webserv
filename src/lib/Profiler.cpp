#include "lib/Profiler.hpp"
#include <iostream>

Profiler::Profiler(const Profiler& other) { (void)other; }
Profiler& Profiler::operator=(const Profiler& other) { (void)other; return *this; }

Profiler::Profiler(const char *name) : _name(name), _start(std::clock()) {}
Profiler::~Profiler()
{
	std::clock_t end = std::clock();
	double ms = 1000.0 * (end - _start) / CLOCKS_PER_SEC;
	std::cout << _name << ": " << ms << " ms" << std::endl;
}
