#ifndef INCLUDE_SISMICPP_MODEL_CONTEXT
#define INCLUDE_SISMICPP_MODEL_CONTEXT

#include "model/events.h"

#include <memory>

namespace sismicpp {

struct TimeContext {
    virtual bool active(const std::string& name) const = 0;
    virtual bool after() const = 0;
    virtual bool idle() const = 0;
    virtual double get_time() const = 0;
    virtual ~TimeContext() {}
};

struct EventContext {
    virtual void send(std::unique_ptr<InternalEvent> event) = 0;
    virtual void notify(std::unique_ptr<MetaEvent> event) = 0;
    virtual ~EventContext() {}
};

using exec_func = void (*)(Event* event, TimeContext* time_context, EventContext* event_context, void* data_context);
using eval_func = bool (*)(Event* event, TimeContext* time_context, void* data_context);

}  // namespace sismicpp

#endif  // INCLUDE
