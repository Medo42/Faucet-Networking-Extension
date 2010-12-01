#include "HandleManager.h"

HandleManager::HandleManager() : handles(), nextHandle(0) {}

uint32_t HandleManager::createHandle(void *element) {
	while(handles.count(nextHandle) > 0) {
		nextHandle++;
	}
	handles[nextHandle] = element;
	return nextHandle++;
}

template<typename ElementInterface>
ElementInterface *HandleManager::findElement(uint32_t handle) {
	std::map<uint32_t, void*>::iterator iter = handles.find(handle);
	if(iter == handles.end()) {
		return 0;
	} else {
		return dynamic_cast<ElementInterface*>(iter);
	}
}

void HandleManager::deleteHandle(uint32_t handle) {
	handles.erase(handle);
}
