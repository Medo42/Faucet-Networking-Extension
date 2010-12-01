#pragma once
#include <map>
#include <inttypes.h>

class HandleManager {
public:
	HandleManager();
	uint32_t createHandle(void *element);
	template<typename ElementInterface> ElementInterface *findElement(uint32_t handle);
	void deleteHandle(uint32_t handle);

private:
	std::map<uint32_t, void*> handles;
	uint32_t nextHandle;
};
