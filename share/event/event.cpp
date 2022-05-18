#include "event.h"
#include "corpc_utils.h"

uint32_t EventEmitter::addEventHandle(const std::string &name, EventHandle handle) {
    uint32_t handleId = ++_idGen;

    _m1[handleId] = name;
    _m2[name][handleId] = handle;

    return handleId;
}

void EventEmitter::removeEventHandle(uint32_t handleId) {
    auto it = _m1.find(handleId);
    if (it == _m1.end()) {
        WARN_LOG("EventEmitter::removeEventHandle -- handle[%d] not found\n", handleId);
        return;
    }

    auto it1 = _m2.find(it->second);
    assert(it1 != _m2.end());

    it1->second.erase(handleId);
    if (it1->second.empty()) {
        _m2.erase(it1);
    }

    _m1.erase(it);
}

void EventEmitter::fireEvent(const Event &event) {
    auto it = _m2.find(event.getName());
    if (it != _m2.end()) {
        for (auto it1 : it->second) {
            it1.second(event);
        }
    }
}

void EventEmitter::clear() {
    _m1.clear();
    _m2.clear();
}