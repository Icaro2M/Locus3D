#pragma once

#include <vector>
#include <functional>
#include <cstdint>
#include <string>

enum class EventType {
    None,

    DeleteObject,
    RenameObject,

    TransformChanged,
    AddPrimitive,
    AddCustomSolid,

    FileNew,
    FileOpen,
    FileSave,
    FileSaveAs,
    FileExit,

    InputKeyW,
    InputKeyE,
    InputKeyR,
    InputKeyG,
    InputKeyL,
    InputKeyF,
    InputKeyT,
    InputKeyM,
    InputKeyS,
    InputKeyEscape,
    InputKeyEnter,

    InputMouseClickLeft,
    InputMouseReleaseLeft,

    ToolStarted,
    ToolCanceled,
    ToolConfirmed
};

struct Event {
    EventType type;
    uint32_t payloadUInt;
    int payloadInt;
    std::string payloadString;

    Event(EventType t);
    Event(EventType t, uint32_t u);
    Event(EventType t, int i);
    Event(EventType t, uint32_t u, const std::string& s);
};

using EventCallback = std::function<void(const Event&)>;

class AppEventBus {
public:
    AppEventBus() = default;
    ~AppEventBus() = default;

    void subscribe(EventCallback callback);

    void emit(EventType type);
    void emit(EventType type, uint32_t payload);
    void emit(EventType type, int payload);
    void emit(EventType type, uint32_t payload, const std::string& text);

private:
    std::vector<EventCallback> m_subscribers;
};