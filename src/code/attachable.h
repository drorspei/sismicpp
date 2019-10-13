#ifndef INCLUDE_SISMICPP_CODE_ATTACHABLE
#define INCLUDE_SISMICPP_CODE_ATTACHABLE

#include "model/events.h"

namespace sismicpp {

struct Attachable {
    virtual void operator()(MetaEvent* event) = 0;
    virtual ~Attachable() {}
};

}  // namespace sismicpp

#endif  // INCLUDE
