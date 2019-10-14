#ifndef INCLUDE_SISMICPP_MODEL_CONTEXT
#define INCLUDE_SISMICPP_MODEL_CONTEXT

#include "model/events.h"

#include <memory>

namespace sismicpp {

using preamble_func = void (*)(void*);

struct OnEntryExitContext {
    virtual bool active(const std::string& name) const = 0;
    virtual double get_time() const = 0;
    virtual void send(std::unique_ptr<InternalEvent> event) = 0;
    virtual void notify(std::unique_ptr<MetaEvent> event) = 0;
    virtual ~OnEntryExitContext() {}
};

using on_entryexit_func = void (*)(void*, OnEntryExitContext&);

struct GuardContext {
    virtual bool active(const std::string& name) const = 0;
    virtual double get_time() const = 0;
    virtual bool after(double seconds) const = 0;
    virtual bool idle(double seconds) const = 0;
    virtual Event const* get_event() const = 0;
    virtual ~GuardContext() {}
};

using guard_func = bool (*)(const void*, GuardContext&);

struct ActionContext {
    virtual bool active(const std::string& name) const = 0;
    virtual double get_time() const = 0;
    virtual void send(std::unique_ptr<InternalEvent> event) = 0;
    virtual void notify(std::unique_ptr<MetaEvent> event) = 0;
    virtual std::shared_ptr<const Event> get_event() const = 0;
    virtual ~ActionContext() {}
};

using action_func = void (*)(void*, ActionContext&);

}  // namespace sismicpp

#endif  // INCLUDE
