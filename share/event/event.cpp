#include "event.h"
#include "corpc_utils.h"

uint32_t EventEmitter::addEventHandle(const std::string &name, EventHandle handle) {
    uint32_t handleId = ++idGen_;

    m1_[handleId] = name;
    m2_[name][handleId] = handle;

    return handleId;
}

void EventEmitter::removeEventHandle(uint32_t handleId) {
    auto it = m1_.find(handleId);
    if (it == m1_.end()) {
        WARN_LOG("EventEmitter::removeEventHandle -- handle[%d] not found\n", handleId);
        return;
    }

    auto it1 = m2_.find(it->second);
    assert(it1 != m2_.end());

    it1->second.erase(handleId);
    if (it1->second.empty()) {
        m2_.erase(it1);
    }

    m1_.erase(it);
}

void EventEmitter::fireEvent(const Event &event) {
    auto it = m2_.find(event.getName());
    if (it != m2_.end()) {
        for (auto it1 : it->second) {
            it1.second(event);
        }
    }
}

void EventEmitter::clear() {
    m1_.clear();
    m2_.clear();
}