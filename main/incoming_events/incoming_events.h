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

struct GCalConfig_t {
    char url[256];
    char device[64];
    char key[128];
    char days[4];   // "1".."90"
};

struct GTaskConfig_t {
    char url[256];
    char token[128];
};

struct IncomingTask_t {
    std::string title;
};

class IncomingEvents {
public:
    static IncomingEvents& GetInstance();

    void Init();

    // Returns a snapshot copy (thread-safe)
    std::vector<IncomingEvent_t> GetEvents();
    std::vector<IncomingTask_t>  GetTasks();

    // Increments on every successful fetch — callers use this to detect changes
    int GetVersion()      const { return _version; }
    int GetTasksVersion() const { return _tasks_version; }

    // Called from the WiFi config portal save handler
    static void SaveConfig(const GCalConfig_t& cfg);
    static bool LoadConfig(GCalConfig_t& cfg);

    static void SaveGTaskConfig(const GTaskConfig_t& cfg);
    static bool LoadGTaskConfig(GTaskConfig_t& cfg);

private:
    IncomingEvents();
    ~IncomingEvents();
    IncomingEvents(const IncomingEvents&) = delete;
    IncomingEvents& operator=(const IncomingEvents&) = delete;

    static void   EventTask(void* arg);
    bool          FetchAndParse(const char* url);
    bool          FetchAndParseTasks(const char* url, const char* token);
    bool          ParseResponse(const char* json, int len);
    bool          ParseTasksResponse(const char* json, int len);

    std::vector<IncomingEvent_t> _events;
    std::vector<IncomingTask_t>  _tasks;
    SemaphoreHandle_t            _mutex;
    bool                         _initialized;
    volatile int                 _version;
    volatile int                 _tasks_version;
};
