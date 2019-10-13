#include "interpreter/default.h"

int main() {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(std::make_unique<CompoundState>("root", "A"), "");

    Interpreter interp{std::move(statechart)};
}