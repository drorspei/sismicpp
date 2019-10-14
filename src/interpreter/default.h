#ifndef INCLUDE_SISMICPP_INTERPRETER_DEFAULT
#define INCLUDE_SISMICPP_INTERPRETER_DEFAULT

#include "utilities.h"
#include "model/steps.h"
#include "model/elements.h"
#include "model/statechart.h"
#include "model/events.h"
#include "clock/clock.h"
#include "code/attachable.h"
#include "code/evaluator.h"
#include "code/cpp.h"

#include <string>
#include <memory>
#include <utility>
#include <map>
#include <vector>
#include <algorithm>

namespace sismicpp {

struct Interpreter : Observable {
    const StateChart statechart;

    bool initialized = false;
    std::unique_ptr<Clock> clock = std::make_unique<SimulatedClock>();
    std::map<std::string, std::vector<std::string>> memory = {};
    std::vector<std::string> configuration = {};
    std::vector<std::pair<double, std::shared_ptr<const InternalEvent>>> internal_queue = {};
    std::vector<std::pair<double, std::shared_ptr<const Event>>> external_queue = {};
    std::vector<Attachable*> listeners = {};

    std::unique_ptr<Evaluator> evaluator;

    Interpreter(StateChart statechart, void* context) : 
    statechart(std::move(statechart)),
    evaluator(std::make_unique<CppEvaluator>(*this, context)) {
        evaluator->execute_statechart(statechart);
    }

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

    void attach(Attachable* listener) override {
        listeners.push_back(listener);
    }

    void detach(Attachable* listener) override {
        listeners.erase(std::find(listeners.begin(), listeners.end(), listener));
    }

    Interpreter& queue(std::shared_ptr<const Event> event) {
        double time = clock->get_time() + event->delay;

        auto insert = [time] (auto& queue, auto event) {
            std::pair<double, decltype(event)> pair = {time, event};

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
        };

        if (event->is_internal_event()) {
            insert(
                internal_queue,
                std::shared_ptr<const InternalEvent>(std::dynamic_pointer_cast<const InternalEvent>(event))
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

    void raise_event(std::shared_ptr<const MetaEvent> event) {
        for (auto&& listener : listeners) {
            listener->operator()(event);
        }
    }

    void raise_event(std::shared_ptr<const InternalEvent> event) {
        MetaEvent event_sent("event sent", clock->get_time());
        event_sent.event = event;
        raise_event(std::make_shared<const MetaEvent>(event_sent));
        queue(std::move(event));
    }

    void raise_event(std::shared_ptr<const Event> event) {
        if (event->is_internal_event()) {
            raise_event(std::shared_ptr<const InternalEvent>(std::dynamic_pointer_cast<const InternalEvent>(event)));
        } else {
            raise_event(std::shared_ptr<const MetaEvent>(std::dynamic_pointer_cast<const MetaEvent>(event)));
        }
    }

    auto select_event_and_consume() {
        auto select_from_queue = [&] (auto& queue) -> std::shared_ptr<const Event> {
            if (!queue.empty()) {
                if (queue.front().first < clock->get_time()) {
                    auto time_event = std::move(queue.front());
                    queue.erase(queue.begin());
                    return std::move(time_event.second);
                }
            }
            return nullptr;
        };

        auto event = select_from_queue(internal_queue);
        if (!event) {
            event = select_from_queue(external_queue);
        }

        return event;
    }

    auto select_event() const {
        auto select_from_queue = [&] (auto& queue) -> std::shared_ptr<const Event> {
            if (!queue.empty()) {
                if (queue.front().first < clock->get_time()) {
                    return queue.front().second;
                }
            }
            return nullptr;
        };

        auto event = select_from_queue(internal_queue);
        if (!event) {
            event = select_from_queue(external_queue);
        }

        return event;
    }

    std::vector<const Transition*> select_transitions(const Event* event, 
                                                const std::vector<std::string>& states,
                                                bool eventless_first,
                                                bool inner_first) const {
        std::vector<const Transition*> selected_transitions;
        std::vector<const Transition*> considered_transitions;
        std::map<std::string, int> state_depth_cache;

        for (auto& transition : statechart.transitions) {
            if (std::find(states.begin(), states.end(), transition.source) != states.end()) {
                if (transition.event == "" or (event and transition.event == event->name)) {
                    if (state_depth_cache.find(transition.source) == state_depth_cache.end()) {
                        state_depth_cache[transition.source] = statechart.depth_for(transition.source);
                    }

                    considered_transitions.push_back(&transition);
                }
            }
        }

        std::vector<std::string> ignored_states;

        for (auto&& has_event_transitions_pair : sorted_groupby(considered_transitions, [] (auto transition) -> bool {
            return transition->event != "";
        }, !eventless_first)) {
            if (!selected_transitions.empty()) {
                break;
            }

            const Event* exposed_event = has_event_transitions_pair.first ? event : nullptr;

            for (auto&& depth_transition_pair : sorted_groupby(has_event_transitions_pair.second, [&state_depth_cache] (auto transition) {
                return state_depth_cache[transition->source];
            }, inner_first)) {
                for (auto&& source_transitions_pair : sorted_groupby(depth_transition_pair.second, [] (auto transition) {
                    return transition->source;
                }, false)) {
                    if (std::find(ignored_states.begin(), ignored_states.end(), source_transitions_pair.first) != ignored_states.end()) {
                        continue;
                    }

                    bool has_found_transitions = false;

                    for (auto&& priority_transitions_pair : sorted_groupby(source_transitions_pair.second, [] (auto transition) {
                        return transition->priority;
                    }, true)) {
                        for (auto&& transition : priority_transitions_pair.second) {
                            if (!transition->guard or evaluator->evaluate_guard(*transition, exposed_event)) {
                                selected_transitions.push_back(transition);
                                has_found_transitions = true;
                            }
                        }

                        if (has_found_transitions) {
                            if (inner_first) {
                                for (auto&& state : statechart.ancestors_for(source_transitions_pair.first)) {
                                    ignored_states.push_back(state);
                                }
                            } else {
                                for (auto&& state : statechart.descendants_for(source_transitions_pair.first)) {
                                    ignored_states.push_back(state);
                                }
                            }
                            ignored_states.push_back(source_transitions_pair.first);
                            break;
                        }
                    }
                }
            }
        }

        return selected_transitions;
    }

    std::vector<const Transition*> sort_transitions(std::vector<const Transition*> transitions) const {
        // TODO: add throw if there are confliciting transitions or indeterminacies.
        std::sort(transitions.begin(), transitions.end(), [&] (auto t1, auto t2) {
            auto d1 = statechart.depth_for(t1->source);
            auto d2 = statechart.depth_for(t2->source);
            if (d1 != d2) { 
                return d1 > d2;
            } else {
                return t1->source < t2->source;
            }
        });
        return transitions;
    }

    std::vector<MicroStep> create_steps(std::shared_ptr<const Event> event, std::vector<const Transition*> transitions) const {
        std::vector<MicroStep> returned_steps;

        for (auto&& transition : transitions) {
            if (transition->target == "") {
                returned_steps.push_back({
                    .event=event,
                    .transition=transition
                });
                continue;
            }

            auto lca = statechart.least_common_ancestor(transition->source, transition->target);
            auto from_ancestors = statechart.ancestors_for(transition->source);
            auto to_ancestors = statechart.ancestors_for(transition->target);

            std::vector<std::string> exited_states;

            auto last_before_lca = transition->source;
            for (auto&& state : from_ancestors) {
                if (state == lca) {
                    break;
                }
                last_before_lca = state;
            }

            auto descendants = statechart.descendants_for(last_before_lca);
            std::reverse(descendants.begin(), descendants.end());
            for (auto&& descendant : descendants) {
                if (std::find(configuration.begin(), configuration.end(), descendant) != configuration.end()) {
                    exited_states.push_back(std::move(descendant));
                }
            }

            if (std::find(configuration.begin(), configuration.end(), last_before_lca) != configuration.end()) {
                exited_states.push_back(std::move(last_before_lca));
            }

            std::vector<std::string> entered_states{transition->target};
            for (auto&& state : to_ancestors) {
                if (state == lca) {
                    break;
                }
                entered_states.insert(entered_states.begin(), std::move(state));
            }

            returned_steps.push_back({
                .event=event,
                .transition=transition,
                .entered_states=std::move(entered_states),
                .exited_states=std::move(exited_states)
            });
        }

        return returned_steps;
    }

    std::vector<MicroStep> compute_steps_initialized() const {
        auto event = select_event();
        auto transitions = select_transitions(event.get(), configuration, true, true);

        if (transitions.empty()) {
            if (event == nullptr) {
                return {};
            } else {
                return {{.event=event}};
            }
        }

        transitions = sort_transitions(std::move(transitions));

        event = transitions[0]->event == "" ? nullptr : event;

        return create_steps(event, transitions);
    }

    std::vector<MicroStep> compute_steps() {
        if (!initialized) {
            initialized = true;
            return {{.entered_states={statechart.get_root()}}};
        } else {
            return compute_steps_initialized();
        }
    }

    std::unique_ptr<MicroStep> create_stabilization_step(const std::vector<std::string>& names) const {
        auto leaves_names = statechart.leaf_for(names);
        std::sort(leaves_names.begin(), leaves_names.end(), [&] (auto&& n1, auto&& n2) {
            auto d1 = statechart.depth_for(n1);
            auto d2 = statechart.depth_for(n2);
            if (d1 != d2) {
                return d1 > d2;
            } else {
                return n1 < n2;
            }
        });
        
        for (auto&& leaf_name : leaves_names) {
            auto& leaf = statechart.state_for(leaf_name);
            if (leaf.is_final_state() and statechart.parent_for(leaf.name) == statechart.get_root()) {
                return std::make_unique<MicroStep>(MicroStep{
                    .exited_states={leaf.name, statechart.get_root()}
                });
            } else if (leaf.is_history_state()) {
                if (memory.find(leaf.name) != memory.end()) {
                    std::vector<std::string> states_to_enter = memory.at(leaf.name);
                    std::sort(states_to_enter.begin(), states_to_enter.end(), [&] (auto&& s1, auto&& s2) {
                        auto d1 = statechart.depth_for(s1);
                        auto d2 = statechart.depth_for(s2);
                        if (d1 != d2) {
                            return d1 < d2;
                        } else {
                            return s1 < s2;
                        }
                    });
                    return std::make_unique<MicroStep>(MicroStep{
                        .entered_states=std::move(states_to_enter),
                        .exited_states={leaf.name}
                    });
                } else {
                    return std::make_unique<MicroStep>(MicroStep{
                        .entered_states={static_cast<const HistoryState*>(&leaf)->memory},
                        .exited_states={leaf.name}
                    });
                }
            } else if (leaf.is_orthogonal_state()) {
                auto children = statechart.children_for(leaf.name);
                if (!children.empty()) {
                    std::sort(children.begin(), children.end());
                    return std::make_unique<MicroStep>(MicroStep{
                        .entered_states=children
                    });
                }
            } else if (leaf.is_compound_state()) {
                auto initial = static_cast<const CompoundState*>(&leaf)->initial;
                if (initial != "") {
                    return std::make_unique<MicroStep>(MicroStep{
                        .entered_states={initial}
                    });
                }
            }
        }

        return nullptr;
    }

    void apply_step(MicroStep& step) {
        auto active_configuration = configuration;
        std::sort(active_configuration.begin(), active_configuration.end());
        std::vector<std::shared_ptr<const Event>> sent_events;

        for (auto&& state_name : step.exited_states) {
            auto& state = statechart.state_for(state_name);
            for (auto&& sent_event : evaluator->execute_on_exit(state)) {
                sent_events.push_back(std::move(sent_event));
            }

            if (state.is_compound_state()) {
                for (auto&& child_name : statechart.children_for(state_name)) {
                    auto& child = statechart.state_for(child_name);
                    if (child.is_history_state()) {
                        std::vector<std::string> descendants;
                        if (child.is_deep_history_state()) {
                            descendants = statechart.descendants_for(state_name);
                        } else {
                            descendants = statechart.children_for(state_name);
                        }
                        std::sort(descendants.begin(), descendants.end());
                        std::vector<std::string> active;
                        std::set_intersection(
                            active_configuration.begin(), active_configuration.end(),
                            descendants.begin(), descendants.end(),
                            std::back_inserter(active)
                        );
                        memory[child_name] = std::move(active);
                    }
                }
            }

            configuration.erase(std::find(configuration.begin(), configuration.end(), state_name));

            auto state_exited = MetaEvent("state exited", clock->get_time());
            state_exited.state = state_name;
            raise_event(std::make_shared<const MetaEvent>(std::move(state_exited)));
        }

        if (step.transition) {
            for (auto&& sent_event : evaluator->execute_action(*step.transition, step.event.get())) {
                sent_events.push_back(std::move(sent_event));
            }

            auto transition_processed = MetaEvent("transition_processed", clock->get_time());
            transition_processed.source = step.transition->source;
            transition_processed.target = step.transition->target;
            transition_processed.event = step.event;
            raise_event(std::make_shared<const MetaEvent>(std::move(transition_processed)));
        }

        for (auto&& state_name : step.entered_states) {
            auto& state = statechart.state_for(state_name);
            for (auto&& sent_event : evaluator->execute_on_entry(state)) {
                sent_events.push_back(std::move(sent_event));
            }

            configuration.push_back(state_name);

            auto state_entered = MetaEvent("state entered", clock->get_time());
            state_entered.state = state_name;
            raise_event(std::make_shared<const MetaEvent>(std::move(state_entered)));
        }

        for (auto& event : sent_events) {
            raise_event(event);
        }

        step.sent_events = sent_events;
    }

    std::vector<MicroStep> stabilize() {
        std::vector<MicroStep> steps;
        auto step = create_stabilization_step(configuration);
        while (step) {
            apply_step(*step);
            steps.push_back(std::move(*step));
            step = create_stabilization_step(configuration);
        }
        return steps;
    }

    std::unique_ptr<MacroStep> execute_once() {
        std::unique_ptr<MacroStep> macro_step;
        raise_event(std::make_shared<const MetaEvent>(MetaEvent("step started", clock->get_time())));

        auto computed_steps = compute_steps();

        if (!computed_steps.empty()) {
            if (computed_steps[0].event) {
                auto event = select_event_and_consume();
                auto event_consumed = MetaEvent("event consumed", clock->get_time());

                // TODO: fix this memory leak.
                event_consumed.event = event;

                raise_event(std::make_shared<const MetaEvent>(std::move(event_consumed)));
            }

            std::vector<MicroStep> executed_steps;
            for (auto& step : computed_steps) {
                apply_step(step);
                executed_steps.push_back(std::move(step));
                for (auto&& stabilizing_step : stabilize()) {
                    executed_steps.push_back(std::move(stabilizing_step));
                }

                macro_step = std::make_unique<MacroStep>(MacroStep{
                    .time=clock->get_time(),
                    .steps=std::move(executed_steps)
                });
            }
        } else {
            macro_step = nullptr;
        }

        raise_event(std::make_shared<const MetaEvent>(MetaEvent("step ended", clock->get_time())));

        return macro_step;
    }

    std::vector<MacroStep> execute() {
        std::vector<MacroStep> ret;
        
        auto macro_step = execute_once();

        while (macro_step) {
            ret.push_back(std::move(*macro_step));
            macro_step = execute_once();
        }

        return ret;
    }
};

}  // namespace sismicpp

#endif  // INCLUDE
