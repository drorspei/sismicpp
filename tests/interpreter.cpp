#include "interpreter/default.h"

#include <iostream>

int main() {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(std::make_unique<CompoundState>("root", "A"), "");
    statechart.add_state(std::make_unique<BasicState>("A"), "root");
    statechart.add_state(std::make_unique<BasicState>("B"), "root");
    statechart.add_transition({
        .source="A",
        .target="B",
        .event="b"
    });

    Interpreter interp{std::move(statechart), nullptr};
    auto e1 = interp.execute();
    auto e2 = interp.queue("b").execute();
    std::cout << "configuration:\n";
    for (auto&& state : interp.configuration) {
        std::cout << state << '\n';
    }
}