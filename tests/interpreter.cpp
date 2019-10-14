#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include <cassert>
#include <cstdio>

#include "interpreter/default.h"

const auto active_func = [] (auto& interp) {
    return [&interp] (auto&& state) {
        auto configuration = interp.get_configuration();
        return std::find(configuration.begin(), configuration.end(), state) != configuration.end();
    };
};

TEST_CASE( "Enter initial states", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(CompoundState("root", "0"), "");
        statechart.add_state(CompoundState("0", "01"), "root");
            statechart.add_state(OrthogonalState("01"), "0");
                statechart.add_state(CompoundState("010", "0100"), "01");
                    statechart.add_state(BasicState("0100"), "010");
                    statechart.add_state(BasicState("0101"), "010");  // not this one
                statechart.add_state(BasicState("011"), "01");
        statechart.add_state(BasicState("00"), "root");  // not this one

    Interpreter interp{std::move(statechart)};
    interp.execute();

    auto active = active_func(interp);

    REQUIRE( active("root") );
    REQUIRE( active("0") );
    REQUIRE( active("01") );
    REQUIRE( active("010") );
    REQUIRE( active("0100") );
    REQUIRE( active("011") );

    REQUIRE( !active("0101") );
    REQUIRE( !active("00") );
}

TEST_CASE( "Simple transition", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(CompoundState("root", "0"), "");
        statechart.add_state(BasicState("0"), "root");
        statechart.add_state(BasicState("1"), "root");
        statechart.add_transition({
            .source="0",
            .event="go!",
            .target="1"
        });

    Interpreter interp{std::move(statechart)};
    interp.execute();
    interp.queue("go!").execute();

    auto active = active_func(interp);

    REQUIRE( active("root") );
    REQUIRE( active("1") );
    REQUIRE( !active("0") );
}

TEST_CASE( "Simple transition with guard", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(CompoundState("root", "0"), "");
        statechart.add_state(BasicState("0"), "root");
        statechart.add_state(BasicState("1"), "root");
        statechart.add_transition({
            .source="0",
            .event="go!",
            .guard=[] (const void* context, auto) { return *(int*)context == 1; },
            .target="1"
        });

    int context = 0;

    Interpreter interp{std::move(statechart), &context};
    interp.execute();
    auto active = active_func(interp);

    interp.queue("go!").execute();

    REQUIRE( active("0") );

    context = 1;
    
    interp.queue("go!").execute();

    REQUIRE( active("1") );
}


TEST_CASE( "Update context in on exit, action, and on entry", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(CompoundState("root", "0"), "");
        statechart.add_state(BasicState(
            "0",
            [] (void* context, auto) { ((std::vector<int>*)context)->push_back(0); },
            [] (void* context, auto) { ((std::vector<int>*)context)->push_back(1); }
        ), "root");
        statechart.add_state(BasicState(
            "1",
            [] (void* context, auto) { ((std::vector<int>*)context)->push_back(3); },
            [] (void* context, auto) { ((std::vector<int>*)context)->push_back(4); }
        ), "root");
        statechart.add_transition({
            .source="0",
            .event="go!",
            .action=[] (void* context, auto) { ((std::vector<int>*)context)->push_back(2); },
            .target="1"
        });

    std::vector<int> context{};

    Interpreter interp{std::move(statechart), &context};
    interp.execute();

    REQUIRE( context.size() == 1 );
    REQUIRE( context[0] == 0 );

    interp.queue("go!").execute();

    REQUIRE( context.size() == 4 );
    REQUIRE( context[0] == 0 );
    REQUIRE( context[1] == 1 );
    REQUIRE( context[2] == 2 );
    REQUIRE( context[3] == 3 );
}

TEST_CASE( "Simple send", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(CompoundState("root", "0"), "");
        statechart.add_state(BasicState("0"), "root");
        statechart.add_state(BasicState("1"), "root");
        statechart.add_transition({
            .source="0",
            .event="not yet...",
            .action=[] (auto context, ActionContext& action_context) { action_context.send("go!"); }
        });
        statechart.add_transition({
            .source="0",
            .event="go!",
            .target="1"
        });

    Interpreter interp{std::move(statechart)};
    interp.execute();
    interp.queue("not yet...").execute();

    auto active = active_func(interp);

    REQUIRE( active("root") );
    REQUIRE( active("1") );
    REQUIRE( !active("0") );
}

TEST_CASE( "Simple after", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart{"MyStateChart"};
    statechart.add_state(CompoundState("root", "0"), "");
        statechart.add_state(BasicState("0"), "root");
        statechart.add_state(BasicState("1"), "root");
        statechart.add_transition({
            .source="0",
            .event="check after",
            .guard=[] (auto conext, GuardContext& guard_context) { return guard_context.after(1); },
            .target="1"
        });

    Interpreter interp{std::move(statechart)};
    auto& clock = (SimulatedClock&)(*interp.clock);

    interp.queue("check after").execute();

    auto active = active_func(interp);

    REQUIRE( active("root") );
    REQUIRE( active("0") );
    REQUIRE( !active("1") );

    clock.update_time(1);
    interp.queue("check after").execute();

    REQUIRE( active("root") );
    REQUIRE( !active("0") );
    REQUIRE( active("1") );
}
