#ifndef INCLUDE_SISMICPP_MODEL_ELEMENTS
#define INCLUDE_SISMICPP_MODEL_ELEMENTS

#include "model/context.h"

#include <string>
#include <vector>

namespace sismicpp {


struct Contract {
    std::vector<std::string> preconditions = {};
    std::vector<std::string> postconditions = {};
    std::vector<std::string> invariants = {};
};

struct State {
    std::string name;
    on_entryexit_func on_entry = nullptr;
    on_entryexit_func on_exit = nullptr;

    explicit State(std::string name) : name(std::move(name)) {}
    explicit State(std::string name, on_entryexit_func on_entry, on_entryexit_func on_exit) :
    name(std::move(name)),
    on_entry{on_entry},
    on_exit{on_exit} {}

    virtual bool is_actions_state() const = 0;
    virtual bool is_transitions_state() const = 0;
    virtual bool is_compound_state() const = 0;
    virtual bool is_orthogonal_state() const = 0;
    virtual bool is_shallow_history_state() const = 0;
    virtual bool is_deep_history_state() const = 0;
    virtual bool is_final_state() const = 0;

    bool is_composite_state() const {
        return is_compound_state() || is_orthogonal_state();
    };

    bool is_history_state() const {
        return is_shallow_history_state() || is_deep_history_state();
    };

    virtual ~State() {}
};

struct BasicState : State {
    bool is_actions_state() const override { return true;};
    bool is_transitions_state() const override { return true;};
    bool is_compound_state() const override { return false;};
    bool is_orthogonal_state() const override { return false;};
    bool is_shallow_history_state() const override { return false;};
    bool is_deep_history_state() const override { return false;};
    bool is_final_state() const override { return false; };

    explicit BasicState(std::string name) :
    State(std::move(name)) {}

    BasicState(std::string name, on_entryexit_func on_entry, on_entryexit_func on_exit) :
    State(std::move(name), on_entry,  on_exit) {}
};

struct CompoundState : State {
    bool is_actions_state() const override { return true;};
    bool is_transitions_state() const override { return true;};
    bool is_compound_state() const override { return true;};
    bool is_orthogonal_state() const override { return false;};
    bool is_shallow_history_state() const override { return false;};
    bool is_deep_history_state() const override { return false;};
    bool is_final_state() const override { return false; };

    std::string initial;

    CompoundState(std::string name, std::string initial) :
    State(std::move(name)),
    initial(std::move(initial)) {}

    CompoundState(std::string name, std::string initial, on_entryexit_func on_entry, on_entryexit_func on_exit) :
    State(std::move(name), on_entry, on_exit),
    initial(std::move(initial)) {}
};

struct OrthogonalState : State {
    bool is_actions_state() const override { return true;};
    bool is_transitions_state() const override { return true;};
    bool is_compound_state() const override { return false;};
    bool is_orthogonal_state() const override { return true;};
    bool is_shallow_history_state() const override { return false;};
    bool is_deep_history_state() const override { return false;};
    bool is_final_state() const override { return false; };

    explicit OrthogonalState(std::string name) :
    State(std::move(name)) {}

    OrthogonalState(std::string name, on_entryexit_func on_entry, on_entryexit_func on_exit) :
    State(std::move(name), on_entry,  on_exit) {}
};

struct HistoryState : State {
    bool is_actions_state() const override { return true;};
    bool is_transitions_state() const override { return false;};
    bool is_compound_state() const override { return false;};
    bool is_orthogonal_state() const override { return false;};
    bool is_shallow_history_state() const override = 0;
    bool is_deep_history_state() const override = 0;
    bool is_final_state() const override { return false; };  

    std::string memory = "";

    explicit HistoryState(std::string name, std::string memory) :
    State(std::move(name)),
    memory(std::move(memory)) {}
};

struct ShallowHistoryState : HistoryState {
    bool is_shallow_history_state() const override { return true;};
    bool is_deep_history_state() const override { return false;};

    explicit ShallowHistoryState(std::string name, std::string memory) :
    HistoryState(std::move(name), std::move(memory)) {}
};

struct DeepHistoryState : HistoryState {
    bool is_shallow_history_state() const override { return false;};
    bool is_deep_history_state() const override { return true;};
    
    explicit DeepHistoryState(std::string name, std::string memory) :
    HistoryState(std::move(name), std::move(memory)) {}
};

struct FinalState : State {
    bool is_actions_state() const override { return true;};
    bool is_transitions_state() const override { return false;};
    bool is_compound_state() const override { return false;};
    bool is_orthogonal_state() const override { return false;};
    bool is_shallow_history_state() const override { return false;};
    bool is_deep_history_state() const override { return false;};
    bool is_final_state() const override { return true; };

    explicit FinalState(std::string name) :
    State(std::move(name)) {}
};

struct Transition {
    std::string source;
    std::string target = "";
    std::string event = "";
    guard_func guard = nullptr;
    action_func action = nullptr;
    int32_t priority = 0;

    bool is_internal() const {
        return target == "";
    }

    bool is_eventless() const {
        return event == "";
    }
};

}  // namespace sismicpp

#endif  // INCLUDE
