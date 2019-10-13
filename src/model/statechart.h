#include "model/elements.h"
#include "exceptions.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

#ifndef INCLUDE_SISMICPP_MODEL_STATECHART
#define INCLUDE_SISMICPP_MODEL_STATECHART

namespace sismicpp {

struct StateChart {
    std::string name;
    std::string description = "";
    exec_func preamble = nullptr;
    std::map<std::string, std::unique_ptr<State>> states = {};
    std::map<std::string, std::string> parent = {};
    std::map<std::string, std::vector<std::string>> children = {{"", {}}};
    std::vector<Transition> transitions = {};

    std::string get_root() const {
        for (const auto& pair : parent) {
            if (pair.second == "") {
                return pair.first;
            }
        }
        return "";
    }

    std::vector<std::string> get_states() const {
        std::vector<std::string> ret;
        ret.reserve(states.size());
        for (const auto& pair : states) {
            ret.push_back(pair.first);
        }

        return ret;
    }

    State& state_for(const std::string& name) {
        return *states.at(name);
    }

    const State& state_for(const std::string& name) const {
        return *states.at(name);
    }

    std::string parent_for(const std::string& name) const {
        return parent.at(name);
    }

    std::vector<std::string> children_for(const std::string& name) const {
        return children.at(name);
    }

    std::vector<std::string> ancestors_for(const std::string& name) const {
        std::vector<std::string> ancestors;
        std::string curr_parent = parent.at(name);
        while (curr_parent != "") {
            ancestors.push_back(curr_parent);
            curr_parent = parent.at(curr_parent);
        }

        return ancestors;
    }

    std::vector<std::string> descendants_for(std::string name) const {
        std::vector<std::string> descendants;
        std::vector<std::string> states_to_consider = {std::move(name)};
        while (!states_to_consider.empty()) {
            name = std::move(states_to_consider.front());
            states_to_consider.erase(states_to_consider.begin());

            for (auto&& child : children.at(name)) {
                states_to_consider.push_back(child);
                descendants.push_back(child);
            }
        }

        return descendants;
    }

    size_t depth_for(const std::string& name) const {
        return ancestors_for(name).size() + 1;
    }

    std::string least_common_ancestor(const std::string& name_first, const std::string& name_second) const {
        auto s1_anc = ancestors_for(name_first);
        auto s2_anc = ancestors_for(name_second);
        for (auto&& state : s1_anc) {
            if (std::find(s2_anc.begin(), s2_anc.end(), state) != s2_anc.end()) {
                return state;
            }
        }

        return "";
    }

    template <typename Iterable>
    std::vector<std::string> leaf_for(const Iterable& names_it) const {
        std::vector<std::string> leaves;
        std::vector<std::string> names;
        for (auto&& name : names_it) {
            names.push_back(name);
        }

        for (auto&& name : names) {
            auto descendants = descendants_for(name);
            if (std::all_of(descendants.begin(), descendants.end(), [&names] (auto&& descendant) {
                    return std::find(names.begin(), names.end(), descendant) == names.end();
                })) {
                leaves.push_back(name);
            }
        }

        return leaves;
    }

    std::vector<const Transition*> transitions_from(const std::string& source) const {
        std::vector<const Transition*> ret;
        for (auto& transition : transitions) {
            if (transition.source == source) {
                ret.push_back(&transition);
            }
        }
        return ret;
    }

    std::vector<Transition*> transitions_from(const std::string& source) {
        std::vector<Transition*> ret;
        for (auto& transition : transitions) {
            if (transition.source == source) {
                ret.push_back(&transition);
            }
        }
        return ret;
    }

    std::vector<const Transition*> transitions_to(const std::string& target) const {
        std::vector<const Transition*> ret;
        for (auto& transition : transitions) {
            if (transition.target == target || (
                transition.target == "" && transition.source == target
            )) {
                ret.push_back(&transition);
            }
        }
        return ret;
    }

    std::vector<Transition*> transitions_to(const std::string& target) {
        std::vector<Transition*> ret;
        for (auto& transition : transitions) {
            if (transition.target == target || (
                transition.target == "" && transition.source == target
            )) {
                ret.push_back(&transition);
            }
        }
        return ret;
    }

    std::vector<const Transition*> transitions_with(const std::string& event) const {
        std::vector<const Transition*> ret;
        for (auto& transition : transitions) {
            if (transition.event == event) {
                ret.push_back(&transition);
            }
        }
        return ret;
    }

    std::vector<Transition*> transitions_with(const std::string& event) {
        std::vector<Transition*> ret;
        for (auto& transition : transitions) {
            if (transition.event == event) {
                ret.push_back(&transition);
            }
        }
        return ret;
    }

    std::vector<std::string> events_for(const std::vector<std::string>& states) const {
        std::vector<std::string> ret;
        for (auto&& state : states) {
            for (auto&& transition : transitions_from(state)) {
                if (transition->event != "") {
                    ret.push_back(transition->event);
                }
            }
        }
        return ret;
    }

    std::vector<std::string> events_for(const std::string& state) const {
        if (name == "") {
            return events_for(get_states());
        } else {
            return events_for(std::vector<std::string>{name});
        }
    }

    std::vector<std::string> events_for() const {
        return events_for(get_states());
    }

    void add_transition(Transition transition) {
        auto& from_state = state_for(transition.source);

        if (!from_state.is_transitions_state()) {
            throw statechart_error("Cannot add transition on state " + from_state.name);
        }

        if (transition.target != "" and states.find(transition.target) == states.end()) {
            throw statechart_error("Unknown target state " + transition.target);
        }

        transitions.push_back(std::move(transition));
    }

    void add_state(std::unique_ptr<State> state, std::string parent) {
        if (state->name == "") {
            throw statechart_error("State must have a name");
        }

        if (states.find(state->name) != states.end()) {
            throw statechart_error("State " + state->name + " already exists!");
        }

        if (parent == "") {
            if (get_root() != "") {
                throw statechart_error("Root already defined. Try adding the state with an existing parent.");
            }
        } else {
            if (states.find(parent) == states.end()) {
                throw statechart_error("Parent '" + parent + "' of '" + state->name + "' does not exist!");
            }

            auto& parent_state = state_for(parent);

            if (!parent_state.is_composite_state()) {
                throw statechart_error("State '" + parent_state.name + "' cannot be used as a parent for '" + state->name + "'");
            }

            if (state->is_history_state() and !parent_state.is_compound_state()) {
                throw statechart_error("State '" + parent_state.name + "' cannot be used as a parent for '" + state->name + "'");
            }

            this->parent[state->name] = parent;
            children[state->name] = {};
            children[parent].push_back(state->name);
            states[state->name] = std::move(state);
        }
    }

    void validate_compoundstate_initial() const {
        for (auto&& pair : states) {
            if (pair.second->is_compound_state()) {
                auto compound_state = static_cast<const CompoundState*>(pair.second.get());
                
                if (states.find(compound_state->initial) == states.end()) {
                    throw statechart_error("Initial state '" + compound_state->initial + "' of state '" + compound_state->name + "' does not exist");
                }

                auto children = children_for(compound_state->name);
                if (std::find(children.begin(), children.end(), compound_state->initial) == children.end()) {
                    throw statechart_error("Initial state '" + compound_state->initial + "' of state '" + compound_state->name + "' must be a child state");
                }
            }
        }
    }

    void validate_historystate_memory() const {
        for (auto&& pair : states) {
            if (pair.second->is_history_state()) {
                auto history_state = static_cast<const HistoryState*>(pair.second.get());
                auto& memory = history_state->memory;

                if (memory == "") {
                    continue;
                }

                if (memory == history_state->name) {
                    throw statechart_error("Initial memory of '" + memory + "' of state '" + history_state->name + "' cannot target itself");
                }

                if (states.find(memory) == states.end()) {
                    throw statechart_error("Initial memory of '" + memory + "' of state '" + history_state->name + "' does not exist");
                }

                auto children = children_for(parent_for(history_state->name));
                if (std::find(children.begin(), children.end(), memory) == children.end()) {
                    throw statechart_error("Initial memory of '" + memory + "' of state '" + history_state->name + "' must be a parent's child");
                }
            }
        }
    }

    void validate() const {
        validate_compoundstate_initial();
        validate_historystate_memory();
    }
};

}  // namespace sismicpp

#endif  // INCLUDE
