#include "AppEventBus.h"

Event::Event(EventType t)
    : type(t),
    payloadUInt(0),
    payloadInt(0),
    payloadString()
{
}

Event::Event(EventType t, uint32_t u)
    : type(t),
    payloadUInt(u),
    payloadInt(0),
    payloadString()
{
}

Event::Event(EventType t, int i)
    : type(t),
    payloadUInt(0),
    payloadInt(i),
    payloadString()
{
}

Event::Event(EventType t, uint32_t u, const std::string& s)
    : type(t),
    payloadUInt(u),
    payloadInt(0),
    payloadString(s)
{
}

void AppEventBus::subscribe(EventCallback callback)
{
    m_subscribers.push_back(callback);
}

void AppEventBus::emit(EventType type)
{
    Event e(type);

    for (auto& callback : m_subscribers) {
        callback(e);
    }
}

void AppEventBus::emit(EventType type, uint32_t payload)
{
    Event e(type, payload);

    for (auto& callback : m_subscribers) {
        callback(e);
    }
}

void AppEventBus::emit(EventType type, int payload)
{
    Event e(type, payload);

    for (auto& callback : m_subscribers) {
        callback(e);
    }
}

void AppEventBus::emit(EventType type, uint32_t payload, const std::string& text)
{
    Event e(type, payload, text);

    for (auto& callback : m_subscribers) {
        callback(e);
    }
}