/**
 * @file event_bus.cpp
 * @brief EventBus unsubscribe/clear.
 */
#include <aether/core/event_bus.hpp>

#include <algorithm>

namespace aether::core {

void EventBus::unsubscribe(ListenerId id) {
    std::lock_guard lock(mutex_);
    for (auto& [type, vec] : listeners_) {
        (void)type;
        const auto it = std::remove_if(vec.begin(), vec.end(),
                                       [id](const Entry& e) { return e.id == id; });
        if (it != vec.end()) {
            vec.erase(it, vec.end());
            return;
        }
    }
}

void EventBus::clear() {
    std::lock_guard lock(mutex_);
    listeners_.clear();
}

} // namespace aether::core
