#ifndef INCLUDE_SISMICPP_INTERPRETER_DEFAULT
#define INCLUDE_SISMICPP_INTERPRETER_DEFAULT

#include "model/steps.h"
#include "model/elements.h"
#include "model/statechart.h"
#include "model/events.h"
#include "clock/clock.h"
#include "code/attachable.h"

#include <string>
#include <memory>
#include <utility>
#include <map>
#include <vector>
#include <algorithm>

namespace sismicpp {

struct Interpreter {
    const StateChart statechart;
    bool initialized = false;
    std::unique_ptr<Clock> clock = std::make_unique<SimulatedClock>();
    std::map<std::string, std::vector<std::string>> memory = {};
    std::vector<std::string> configuration = {};
    std::vector<std::pair<double, std::unique_ptr<InternalEvent>>> internal_queue = {};
    std::vector<std::pair<double, std::unique_ptr<Event>>> external_queue = {};
    std::vector<Attachable*> listeners = {};

    std::vector<std::string> get_configuration() const {
        std::vector<std::string> ret = configuration;
        std::sort(ret.begin(), ret.end(), [&] (auto& s1, auto& s2) -> bool {
            auto d1 = statechart.depth_for(s1);
            auto d2 = statechart.depth_for(s2);
            if (d1 != d2) {
                return d1 < d2;
            } else {
                return s1 < s2;
            }
        });
        return ret;
    }

    bool is_in_final() const {
        return initialized and configuration.empty();
    }

    void attach(Attachable* listener) {
        listeners.push_back(listener);
    }

    void detach(Attachable* listener) {
        listeners.erase(std::find(listeners.begin(), listeners.end(), listener));
    }

    Interpreter& queue(std::unique_ptr<Event> event) {
        auto time = clock->get_time() + event->delay;

        auto insert = [time] (auto& queue, auto event) {
            std::pair<double, decltype(event)> pair = {time, decltype(event){event.get()}};

            queue.insert(
                std::upper_bound(
                    queue.begin(), queue.end(), pair, [] (auto& p1, auto& p2) -> bool {
                        if (p1.first != p2.first) {
                            return p1.first < p2.first;
                        } else {
                            return !p1.second->is_internal_event() < !p2.second->is_internal_event();
                        }
                    }
                ),
                {time, std::move(event)}
            );

            pair.second.release();
        };

        if (event->is_internal_event()) {
            insert(
                internal_queue,
                std::unique_ptr<InternalEvent>(static_cast<InternalEvent*>(event.release()))
            );
        } else {
            insert(external_queue, std::move(event));
        }

        return *this;
    }

    Interpreter& queue(std::string name) {
        queue(std::make_unique<Event>(std::move(name)));
        return *this;
    }

    void raise_event(MetaEvent event) {
        for (auto&& listener : listeners) {
            listener->operator()(&event);
        }
    }

    void raise_event(InternalEvent event) {
        std::unique_ptr<Event> event_ptr = std::make_unique<InternalEvent>(event);
        MetaEvent event_sent("event sent", clock->get_time());
        event_sent.event = event_ptr.get();
        raise_event(event_sent);
        queue(std::move(event_ptr));
    }

    void raise_event(Event)

    std::unique_ptr<MacroStep> execute_once() {
        raise_event(MetaEvent("step started", clock->get_time()));
    }

    std::vector<MacroStep> execute() {
        std::vector<MacroStep> ret;
        
        auto macro_step = execute_once();

        while (macro_step) {
            auto& step = *macro_step.release();
            ret.push_back(std::move(step));
            macro_step = execute_once();
        }

        return ret;
    }
};

}  // namespace sismicpp

#endif  // INCLUDE
