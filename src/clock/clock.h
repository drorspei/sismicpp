#include <stdexcept>
#include <string>

#ifndef INCLUDE_SISMICPP_CLOCK_CLOCK
#define INCLUDE_SISMICPP_CLOCK_CLOCK

namespace sismicpp {

struct Clock {
    virtual double get_time() const = 0;
    virtual ~Clock() {}
};

struct SimulatedClock : Clock {
    double time = 0;

    double get_time() const override {
        return time;
    }

    void set_time(double new_time) {
        if (new_time < time) {
            throw std::runtime_error("Time must be monotic, cannot change time from " + std::to_string(time) + " to " + std::to_string(new_time));
        }
        time = new_time;
    }

    void update_time(double add) {
        time += add;
    }
};

}

#endif  // INCLUDE
