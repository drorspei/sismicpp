#ifndef INCLUDE_SISMICPP_MODEL_STEPS
#define INCLUDE_SISMICPP_MODEL_STEPS

#include "model/events.h"
#include "model/elements.h"

#include <vector>
#include <string>
#include <memory>

namespace sismicpp {

struct MicroStep {
    std::unique_ptr<Event> event = {};
    std::unique_ptr<Transition> transition = {};
    std::vector<std::string> entered_events = {};
    std::vector<std::string> exited_states = {};
    std::vector<Event> sent_events = {};
};

struct MacroStep {
    double time;
    std::vector<MicroStep> steps;
};

}  // namespace sismicpp

#endif  // INCLUDE
