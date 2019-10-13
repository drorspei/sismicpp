#include <stdexcept>

#ifndef INCLUDE_SISMICPP_EXCEPTIONS
#define INCLUDE_SISMICPP_EXCEPTIONS

namespace sismicpp {

struct sismic_error : std::runtime_error {
    explicit sismic_error(const std::string& what_arg) : runtime_error(what_arg) {}
    explicit sismic_error(const char* what_arg ) : std::runtime_error(what_arg) {}
};

struct statechart_error : sismic_error {
    explicit statechart_error(const std::string& what_arg) : sismic_error(what_arg) {}
    explicit statechart_error(const char* what_arg ) : sismic_error(what_arg) {}
};

}  // namespace sismicpp

#endif // INCLUDE