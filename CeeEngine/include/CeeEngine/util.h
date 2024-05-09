#ifndef CEE_ENGINE_UTIL_H_
#define CEE_ENGINE_UTIL_H_

#include <string>

namespace cee {
std::string GetEnvironmentVariable(const std::string& name);
bool HasEnvironmentVariable(const std::string& name);
// Returns 0 on success, otherwise a negative error code.
int32_t SetEnvironmentVariable(const std::string& name, const std::string& val);
}

#endif
