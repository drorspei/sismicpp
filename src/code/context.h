#include "model/events.h"
#include "code/attachable.h"

#include <algorithm>
#include <memory>
#include <string>
#include <map>
#include <vector>

#ifndef INCLUDE_SISMICPP_CODE_CONTEXT
#define INCLUDE_SISMICPP_CODE_CONTEXT

namespace sismicpp {

struct TimeContextProvider : Attachable {
    std::map<std::string, double> entry_time = {};
    std::map<std::string, double> idle_time = {};
    double time = 0;
    std::vector<std::string> configuration = {};

    bool after(const std::string& name, double seconds) const {
        return time - seconds >= entry_time.at(name);
    }

    bool idle(const std::string& name, double seconds) {
        return time - seconds >= idle_time.at(name);
    }

    bool active(const std::string& name) {
        return std::find(configuration.begin(), configuration.end(), name) != configuration.end();
    }

    void operator()(std::shared_ptr<const MetaEvent> event) override {
        if (event->name == "step started") {
            time = event->time;
        } else if (event->name == "state entered") {
            configuration.push_back(event->name);
            entry_time[event->state] = time;
            idle_time[event->state] = time;
        } else if (event->name == "state exited") {
            configuration.erase(std::find(configuration.begin(), configuration.end(), event->state));
        } else if (event->name == "transition processed") {
            idle_time[event->source] = time;
        }
    }
};

struct EventContextProvider : Attachable {
    std::vector<std::unique_ptr<const Event>> pending = {};
    std::vector<std::shared_ptr<const Event>> sent = {};
    std::shared_ptr<const Event> consumed = nullptr;

    void send(std::unique_ptr<InternalEvent> event) {
        pending.push_back(std::move(event));
    }

    void notify(std::unique_ptr<MetaEvent> event) {
        pending.push_back(std::move(event));
    }

    bool was_sent(const std::string& name) const {
        return std::any_of(
            sent.begin(), sent.end(), [&name] (auto&& e) {
                return name == e->name;
            }
        );
    }

    bool received(const std::string& name) const {
        return consumed and consumed->name == name;
    }

    void operator()(std::shared_ptr<const MetaEvent> event) override {
        if (event->name == "event consumed") {
            consumed = event->event;
        } else if (event->name == "event sent") {
            sent.push_back(event->event);
        } else if (event->name == "step started") {
            consumed = nullptr;
            sent.clear();
            pending.clear();
        }
    }
};

}  // namespace sismicpp

#endif  // INCLUDE
