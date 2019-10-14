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
    TimeContextProvider time_provider = {};

    CppEvaluator(Observable& interpreter, void* context) :
    context(context),
    time_provider{} {
        interpreter.attach(&time_provider);
    }

    void* get_context() override {
        return context;
    };

    void execute_statechart(const StateChart& statechart) override {
        if (statechart.preamble) {
            statechart.preamble(context);
        }
    };

    bool evaluate_guard(const Transition& transition, const Event* event) const override {
        struct MyGuardContext : GuardContext {
            bool active(const std::string& name) const override {
                return std::find(time_provider.configuration.begin(), time_provider.configuration.end(), name) != time_provider.configuration.end();
            }

            double get_time() const override {
                return time_provider.time;
            }

            bool after(double seconds) const override {
                return time_provider.after(source, seconds);
            }

            bool idle(double seconds) const override {
                return time_provider.idle(source, seconds);
            }

            Event const* get_event() const override {
                return event;
            }

            MyGuardContext(const TimeContextProvider& time_provider, const std::string& source, const Event* event) :
            time_provider(time_provider), 
            source(source),
            event(event) {}
        private:
            const TimeContextProvider& time_provider;
            const std::string& source;
            const Event* event;
        } guard_context(time_provider, transition.source, event);

        return transition.guard(context, guard_context);
    };

    std::vector<std::shared_ptr<const Event>> execute_action(const Transition& transition, std::shared_ptr<const Event> event) const override {
        std::vector<std::shared_ptr<const Event>> ret;

        struct MyActionContext : ActionContext {
            bool active(const std::string& name) const override {
                return std::find(time_provider.configuration.begin(), time_provider.configuration.end(), name) != time_provider.configuration.end();
            }

            double get_time() const override {
                return time_provider.time;
            }

            void send(Event event) override {
                ret.push_back(std::make_shared<InternalEvent>(std::move(event)));
            }

            void notify(Event event) override {
                ret.emplace_back(std::make_shared<MetaEvent>(std::move(event)));
            }

            std::shared_ptr<const Event> get_event() const override {
                return event;
            }

            MyActionContext(const TimeContextProvider& time_provider,
                            std::shared_ptr<const Event> event,
                            std::vector<std::shared_ptr<const Event>>& ret) :
            time_provider(time_provider), 
            event(event),
            ret(ret) {}
        private:
            const TimeContextProvider& time_provider;
            std::shared_ptr<const Event> event;
            std::vector<std::shared_ptr<const Event>>& ret;
        } action_context(time_provider, event, ret);

        transition.action(context, action_context);

        return ret;
    };

    std::vector<std::shared_ptr<const Event>> execute_on_entryexit(on_entryexit_func func) const {
        std::vector<std::shared_ptr<const Event>> ret;

        struct MyOnEntryExitContext : OnEntryExitContext {
            bool active(const std::string& name) const override {
                return std::find(time_provider.configuration.begin(), time_provider.configuration.end(), name) != time_provider.configuration.end();
            }

            double get_time() const override {
                return time_provider.time;
            }

            void send(Event event) override {
                ret.emplace_back(std::make_shared<InternalEvent>(std::move(event)));
            }

            void notify(Event event) override {
                ret.emplace_back(std::make_shared<MetaEvent>(std::move(event)));
            }

            MyOnEntryExitContext(const TimeContextProvider& time_provider,
                            std::vector<std::shared_ptr<const Event>>& ret) :
            time_provider(time_provider), 
            ret(ret) {}
        private:
            const TimeContextProvider& time_provider;
            std::vector<std::shared_ptr<const Event>>& ret;
        } on_entryexit_context(time_provider, ret);

        func(context, on_entryexit_context);

        return ret;
    };

    std::vector<std::shared_ptr<const Event>> execute_on_entry(const State& state) const override {
        return execute_on_entryexit(state.on_entry);
    }

    std::vector<std::shared_ptr<const Event>> execute_on_exit(const State& state) const override {
        return execute_on_entryexit(state.on_exit);
    };
};

}  // namespace sismicpp

#endif  // INCLUDE
