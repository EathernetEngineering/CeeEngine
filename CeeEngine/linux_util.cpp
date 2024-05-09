#include <CeeEngine/util.h>

#include <cstdlib>
#include <cstring>

#include <errno.h>

namespace cee {	
std::string GetEnvironmentVariable(const std::string& name) {
	const char* env = std::getenv(name.c_str());
	if (env == nullptr) {
		return {};
	}
	else {
		return std::string(env);
	}
}

bool HasEnvironmentVariable(const std::string& name) {
	return !GetEnvironmentVariable(name).empty();
}

int32_t SetEnvironmentVariable(const std::string& name, const std::string& val) {
	// NOTE env will be `name=var`, so length is name.length() + "=".length() [1] + val.length() + "\0".length()
	// totaling the equation below.
	char* env = reinterpret_cast<char*>(malloc(name.length() + 1 + val.length() + 1));
	if (env == nullptr) {
		return -ENOMEM;
	}
	strcpy(env, name.c_str()); 
	strcat(env, "=");
	strcat(env, val.c_str());
	// NOTE the value passed into `putenv` becomes the environment, so we shouldn't free it.
	putenv(env);
	return 0;
}
}
