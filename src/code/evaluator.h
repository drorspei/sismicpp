#ifndef INCLUDE_SISMICPP_CODE_EVALUATOR
#define INCLUDE_SISMICPP_CODE_EVALUATOR

#include "code/context.h"

#include <string>

namespace sismicpp {

struct Evaluator {
    void* context = nullptr;
    EventContextProvider event_provider = {};
    TimeContextProvider time_provider = {};
};

}  // namespace sismicpp

#endif  // INCLUDE
