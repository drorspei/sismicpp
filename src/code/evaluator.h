#ifndef INCLUDE_SISMICPP_CODE_EVALUATOR
#define INCLUDE_SISMICPP_CODE_EVALUATOR

#include "code/context.h"
#include "model/elements.h"
#include "model/statechart.h"

#include <string>
#include <vector>
#include <memory>

namespace sismicpp {

struct Evaluator {
    virtual void* get_context() = 0;
    virtual void execute_statechart(const StateChart& statechart) = 0;
    virtual bool evaluate_guard(const Transition& transition, const Event* event) const = 0;
    virtual std::vector<std::shared_ptr<const Event>> execute_action(const Transition& transition, std::shared_ptr<const Event> event) const = 0;
    virtual std::vector<std::shared_ptr<const Event>> execute_on_entry(const State& state) const = 0;
    virtual std::vector<std::shared_ptr<const Event>> execute_on_exit(const State& state) const = 0;
    virtual ~Evaluator() {}
};

}  // namespace sismicpp

#endif  // INCLUDE
