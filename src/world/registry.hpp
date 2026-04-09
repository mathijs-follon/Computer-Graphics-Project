#ifndef CG_OPENGL_PROJECT_REGISTRY_H
#define CG_OPENGL_PROJECT_REGISTRY_H
#include <algorithm>
#include <any>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "events.hpp"

class Registry {
public:
    Registry() = default;

    void registerObject(std::string name, auto object) {
        m_objects[std::move(name)] = std::move(object);
    }

    template <typename T>
    [[nodiscard]] bool hasObject(const std::string& name) const {
        if (const auto it = m_objects.find(name); it != m_objects.end()) {
            // If found an object with name, return true if the found type matches the requested type
            return it->second.type() == typeid(T);
        }
        return false;
    }

    template <typename T>
    [[nodiscard]] T* getObject(const std::string& name) {
        if (const auto it = m_objects.find(name); it != m_objects.end()) {
            // Cast the std::any type to the requested type
            return std::any_cast<T>(&it->second);
        }
        return nullptr;
    }

    template <typename T>
    [[nodiscard]] const T* getObject(const std::string& name) const {
        if (const auto it = m_objects.find(name); it != m_objects.end()) {
            return std::any_cast<const T>(&it->second);
        }
        return nullptr;
    }

    template <typename T>
    auto getObjects() {
        return m_objects | std::views::values |
                // Cast all to the requested T type
               std::views::transform([](std::any& value) { return std::any_cast<T>(&value); }) |
                // Filter out failed casts (nullptr's)
               std::views::filter([](const T* value) { return value != nullptr; });
    }


    template <typename T>
    auto getObjects() const {
        return m_objects | std::views::values |
            std::views::transform([](const std::any& value) { return std::any_cast<const T>(&value); }) |
            std::views::filter([](const T* value) { return value != nullptr; });
    }

    void emitEvent(const Event event) { m_events.push_back(event); }

    template <typename EventPayload>
    void emitEvent(EventPayload payload) {
        static_assert(std::is_enum_v<decltype(EventTraits<EventPayload>::type)>,
                      "EventTraits must define a valid EventType");
        // Uses traits to define the EventType of a certain EventPayload.
        // Eg. EventTraits<ApplicationExitRequest>::type == EventType::ApplicationExitRequest
        emitEvent(Event{EventTraits<EventPayload>::type, std::move(payload)});
    }

    [[nodiscard]] bool hasEvent(const EventType type) const {
        // Check if any event.type in range equals type
        return std::ranges::any_of(m_events, [type](const Event& event) { return event.type == type; });
    }

    std::optional<Event> pollEvents(EventType type) {
        // Find the first matching event on type
        const auto it = std::ranges::find_if(
            m_events, [type](const Event& event) { return event.type == type; });

        // If there was a match then return and erase it
        if (it != m_events.end()) {
            Event event = *it;
            m_events.erase(it);
            return event;
        }
        return std::nullopt;
    }

    void flushEvents() { m_events.clear(); }

private:
    std::unordered_map<std::string, std::any> m_objects;
    std::vector<Event> m_events;
};

#endif  // CG_OPENGL_PROJECT_REGISTRY_H