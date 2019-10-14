#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include <cassert>
#include <cstdio>

#include "model/builder.h"

TEST_CASE( "Enter initial states", "[sismicpp]" ) {
    using namespace sismicpp;

    StateChart statechart = [] {
        using namespace sismicpp::builder;
        return build_statechart(
            name("My Button StateChart"),
            description("A showcase of statecharts and buttons"),
            state(
                name("Button"),
                initial("Off"),
                state(
                    name("Off"),
                    transition(
                        event("toggle"),
                        target("On")
                    )
                ),
                state(
                    name("On"),
                    transition(
                        event("toggle"),
                        target("Off")
                    )
                )
            )
        );
    }();

    REQUIRE( statechart.name == "My Button StateChart" );
    REQUIRE( statechart.description == "A showcase of statecharts and buttons" );
    REQUIRE( statechart.get_states().size() == 3 );
    REQUIRE( statechart.get_root() == "Button" );
    REQUIRE( statechart.parent_for("On") == "Button" );
    REQUIRE( statechart.parent_for("Off") == "Button" );

    auto children = statechart.children_for("Button");
    REQUIRE( children.size() == 2 );
    REQUIRE( children[0] == "Off" );
    REQUIRE( children[1] == "On" );

    auto button = static_cast<CompoundState*>(&statechart.state_for("Button"));
    REQUIRE( button->initial == "Off" );

    auto transitions = statechart.transitions_from("Off");
    REQUIRE( transitions.size() == 1 );
    REQUIRE( (*transitions[0]).event == "toggle" );
    REQUIRE( (*transitions[0]).target == "On" );

    transitions = statechart.transitions_from("On");
    REQUIRE( transitions.size() == 1 );
    REQUIRE( (*transitions[0]).event == "toggle" );
    REQUIRE( (*transitions[0]).target == "Off" );
}
