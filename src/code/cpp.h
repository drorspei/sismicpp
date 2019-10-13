#ifndef INCLUDE_SISMICPP_CODE_CPP
#define INCLUDE_SISMICPP_CODE_CPP

#include "code/context.h"
#include "code/evaluator.h"
#include "code/attachable.h"

#include <memory>
#include <string>

namespace sismicpp {

struct CppEvaluator : Evaluator {
    void* context = nullptr;
    EventContextProvider event_provider = {};
    TimeContextProvider time_provider = {};

    CppEvaluator(Observable& interpreter, void* context) :
    context(context),
    event_provider{},
    time_provider{} {
        interpreter.attach(&event_provider);
        interpreter.attach(&time_provider);
    }

    void* get_context() override {
        return context;
    };

    void execute_statechart(const StateChart& statechart) override {
        
    };

    bool evaluate_guard(const Transition& transition, const Event* event) const override {
        return true;
    };

    std::vector<std::shared_ptr<const Event>> execute_action(const Transition& transition, const Event* event) const override {
        return {};
    };

    std::vector<std::shared_ptr<const Event>> execute_on_entry(const State& state) override {
        return {};
    };

    std::vector<std::shared_ptr<const Event>> execute_on_exit(const State& state) override {
        return {};
    };
};

}  // namespace sismicpp

#endif  // INCLUDE
