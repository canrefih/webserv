#include "lib/StringBuilder.hpp"

StringBuilder::StringBuilder() : _buf(NULL), _len(0), _cap(0) {}
StringBuilder::~StringBuilder() { delete _buf; }
StringBuilder::StringBuilder(const StringBuilder& other) { (void)other; }
StringBuilder& StringBuilder::operator=(const StringBuilder& other) { (void)other; return *this; }
