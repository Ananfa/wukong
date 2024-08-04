#include "event.h"
#include "corpc_utils.h"

void EventEmitter::regEventHandle(const std::string &name, EventHandle handle) {
    eventHandles_[name].push_back(handle);
}

void EventEmitter::fireEvent(const Event &event) {
    auto it1 = eventHandles_.find(event.getName());
    if (it1 != eventHandles_.end()) {
        for (auto it2 : it1->second) {
            it2(event);
        }
    }
}

void EventEmitter::clear() {
    eventHandles_.clear();
}