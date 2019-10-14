#ifndef INCLUDE_SISMICPP_MODEL_BUILDER
#define INCLUDE_SISMICPP_MODEL_BUILDER

#include "model/elements.h"
#include "model/statechart.h"

#define ATTRIBUTE_SETTER(attribute, arg_type) auto attribute = [] (auto attribute) { \
    return [attribute] (arg_type& arg) -> decltype(arg)& {                     \
        arg.attribute = attribute;                                         \
        return arg;                                                        \
    };                                                                     \
};

#define ATTRIBUTE_SETTER_AUTO(attribute) ATTRIBUTE_SETTER(attribute, auto)

namespace sismicpp {
namespace builder {
namespace detail {

template <typename... Args>
void swallow(Args&&...) {}

struct PartialState {
    std::string name = "";
    std::string parent = "";
    std::string initial = "";
    std::string type = "";
    std::string memory = "";
    bool is_orthogonal = false;
    bool is_compound = false;
    on_entryexit_func on_entry = nullptr;
    on_entryexit_func on_exit= nullptr;
    std::vector<Transition> transitions = {};
    std::vector<PartialState> inner_states = {};

    StateChart& operator()(StateChart& statechart) {
        add_all_states(statechart);
        add_all_transitions(statechart);
        return statechart;
    }

    void add_all_states(StateChart& statechart) {
        auto state = [&] () -> std::unique_ptr<State> {
            if (type != "") {
                if (is_orthogonal) {
                    throw statechart_error("Parallel state '" + name + "' cannot also be of type '" + type);
                } else if (is_compound or !inner_states.empty()) {
                    throw statechart_error("Compound state '" + name + "' cannot also be of type '" + type);
                } else if (!transitions.empty()) {
                    throw statechart_error("State '" + name + "' cannot be of type '" + type + " and also have transitions");
                } else if (type == "final") {
                    return std::make_unique<FinalState>(name);
                } else if (type == "shallow history") {
                    return std::make_unique<ShallowHistoryState>(name, memory);
                } else if (type == "deep history") {
                    return std::make_unique<DeepHistoryState>(name, memory);
                }
            } else if (is_orthogonal) {
                return std::make_unique<OrthogonalState>(name);
            } else if (is_compound) {
                return std::make_unique<CompoundState>(name, std::move(initial));
            }

            return std::make_unique<BasicState>(name);
        }();

        state->on_entry = on_entry;
        state->on_exit = on_exit;

        statechart.add_state(std::move(state), parent);

        for (auto&& inner_state : inner_states) {
            inner_state.parent = name;
            inner_state.add_all_states(statechart);
        }
    }

    void add_all_transitions(StateChart& statechart) {
        for (auto&& transition : transitions) {
            transition.source = name;
            statechart.add_transition(std::move(transition));
        }
        
        for (auto&& inner_state : inner_states) {
            inner_state.add_all_transitions(statechart);
        }
    }

    PartialState& operator()(PartialState& parent_state) && {
        parent_state.is_compound = true;
        parent_state.inner_states.push_back(std::move(*this));
        return parent_state;
    }
};
}  // namespace detail

ATTRIBUTE_SETTER_AUTO(name);
ATTRIBUTE_SETTER_AUTO(initial);
ATTRIBUTE_SETTER_AUTO(on_entry);
ATTRIBUTE_SETTER_AUTO(on_exit);

ATTRIBUTE_SETTER(description, StateChart);
ATTRIBUTE_SETTER(preamble, StateChart);

ATTRIBUTE_SETTER(memory, detail::PartialState);

ATTRIBUTE_SETTER(event, Transition);
ATTRIBUTE_SETTER(guard, Transition);
ATTRIBUTE_SETTER(action, Transition);
ATTRIBUTE_SETTER(target, Transition);

auto type = [] (std::string type) {
    if (type != "final" and
        type != "shallow history" and
        type != "deep history") {
        throw sismic_error("State type '" + type + "' is not one of 'final', 'shallow history', or 'deep history'");
    }
    return [type=std::move(type)] (detail::PartialState& partial_state) -> detail::PartialState& {
        partial_state.type = type;
        return partial_state;
    };
};

auto transition = [] (auto&&... args) {
    Transition transition{""};
    detail::swallow(std::forward<decltype(args)>(args)(transition)...);
    return [transition] (detail::PartialState& arg) -> decltype(arg)& {
        arg.transitions.push_back(transition);
        return arg;
    };
};

auto state = [] (auto&&... args) {
    detail::PartialState partial_state;
    detail::swallow(std::forward<decltype(args)>(args)(partial_state)...);
    return partial_state;
};

auto parallel_state = [] (auto&&... args) {
    detail::PartialState partial_state;
    partial_state.is_orthogonal = true;
    detail::swallow(std::forward<decltype(args)>(args)(partial_state)...);
    return partial_state;
};

auto build_statechart = [] (auto&&... args) -> StateChart {
    StateChart ret{""};
    detail::swallow(std::forward<decltype(args)>(args)(ret)...);
    ret.validate();
    return ret;
};

}
}  // namespace sismicpp

#undef ATTRIBUTE_SETTER_AUTO
#undef ATTRIBUTE_SETTER

#endif  // INCLUDE