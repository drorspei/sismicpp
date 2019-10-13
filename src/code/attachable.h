#ifndef INCLUDE_SISMICPP_CODE_ATTACHABLE
#define INCLUDE_SISMICPP_CODE_ATTACHABLE

#include "model/events.h"
#include <memory>

namespace sismicpp {

struct Attachable {
    virtual void operator()(std::shared_ptr<const MetaEvent> event) = 0;
    virtual ~Attachable() {}
};

struct Observable {
    virtual void attach(Attachable* listener) = 0;
    virtual void detach(Attachable* listener) = 0;
    virtual ~Observable() {}
};

}  // namespace sismicpp

#endif  // INCLUDE
