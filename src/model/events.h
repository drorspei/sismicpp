#ifndef INCLUDE_SISMICPP_MODEL_EVENTS
#define INCLUDE_SISMICPP_MODEL_EVENTS

#include <string>
#include <memory>

namespace sismicpp {

struct Event {
    std::string name;
    double delay = 0;
    void* data = nullptr;

    explicit Event(std::string name_) : name(std::move(name_)) {}

    virtual bool is_internal_event() const {
        return false;
    }

    virtual ~Event() {}
};

struct InternalEvent : Event {
    bool is_internal_event() const override {
        return true;
    }
};

struct MetaEvent : Event {
    double time = 0;
    std::string state = "";
    std::string source = "";
    std::string target = "";
    std::shared_ptr<const Event> event = nullptr;

    MetaEvent(std::string name, double time) : Event(std::move(name)), time(time) {}
};

}  // namespace sismicpp

#endif  // INCLUDE
