# Sismic++
## A C++ library for running statecharts

This library is a C++ (14 and above) translation of the python package [Sismic (https://sismic.readthedocs.io/)](https://sismic.readthedocs.io/) by Alexandre Decan and contributors [1].

Sismic (and thus also Sismic++) is an implementation of the **statechart** abstraction and of an interpreter for it. It mostly conforms to the SCXML standard. See here for more on statecharts https://statecharts.github.io/ .

## A simple example

```c++
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

Interpreter interpreter{std::move(statechart)};
interpreter.queue("toggle").execute();
interpreter.queue("toggle").execute();
```

## What lies ahead

* Compile-time everything!

    Currently the library is fully dynamic with lots of standard library constructs. It takes more than a second to compile the simple example on my laptop, and the binary is almost 200kb. The goal is to make everything in compile-time and without the stdlib, like in the amazing libraries [[Boost].SML](https://boost-experimental.github.io/sml/) and [Boost.MSM](https://www.boost.org/doc/libs/1_60_0/libs/msm/doc/HTML/ch03s04.html). I tried starting with template-meta-programming before and failed, and @relvox said I should try doing it dynamically first.

* Tests?

* Documentation...


    I need to add examples of all the constructs and DSL. Basically, the `sismicpp::builder` provides a DSL that resembles the YAML way of defining statcharts in sismic.

## Additional notes

* The library supports C++ 14 or higher.

* There are some tests already written with the library Catch2. The license is included in the file `LICENSE_catch.txt`.


[1] @software{sismic,
  author = {Decan, Alexandre},
  title = {Sismic Interactive Statechart Model Interpreter and Checker},
  url = {https://github.com/AlexandreDecan/sismic},
}