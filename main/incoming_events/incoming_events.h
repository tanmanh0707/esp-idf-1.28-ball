#pragma once
#include "config_app.h"
#include <string>
#include <vector>

struct IncomingEvent_t {
    std::string title;
    std::string start;
    std::string end;
    std::string location;
};

class IncomingEvents {
public:
    static IncomingEvents& GetInstance();

    void Init();

    // Returns a snapshot copy (thread-safe)
    std::vector<IncomingEvent_t> GetEvents();

    // Increments on every successful fetch — callers use this to detect changes
    int GetVersion() const { return _version; }

private:
    IncomingEvents();
    ~IncomingEvents();
    IncomingEvents(const IncomingEvents&) = delete;
    IncomingEvents& operator=(const IncomingEvents&) = delete;

    static void   EventTask(void* arg);
    bool          FetchAndParse();
    bool          ParseResponse(const char* json, int len);

    std::vector<IncomingEvent_t> _events;
    SemaphoreHandle_t            _mutex;
    bool                         _initialized;
    volatile int                 _version;
};
