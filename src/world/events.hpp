#ifndef CG_OPENGL_PROJECT_EVENTS_HPP
#define CG_OPENGL_PROJECT_EVENTS_HPP
#include <variant>

enum class EventType {
    WindowCloseRequested,
    ApplicationExitRequest
};

// EventTraits legt de koppeling tussen een payload type en EventType.
// Voor elk nieuw event voegen we hier een specialization toe.
template <typename T>
struct EventTraits;

// Events:
struct ApplicationExitRequest {};

template <>
struct EventTraits<ApplicationExitRequest> {
    // Wordt gebruikt door Registry::emitEvent(payload)
    // zodat je alleen de payload hoeft door te geven.
    static constexpr auto type = EventType::ApplicationExitRequest;
};

struct Event {
    EventType type;
    std::variant<ApplicationExitRequest
                 // ... andere events
                 >
        event;
};

#endif  // CG_OPENGL_PROJECT_EVENTS_HPP