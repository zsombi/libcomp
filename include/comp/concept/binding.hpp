#ifndef COMP_BINDING_HPP
#define COMP_BINDING_HPP

#include <comp/config.hpp>
#include <comp/concept/property.hpp>

namespace comp
{

template <class DerivedBinding, typename T>
class COMP_TEMPLATE_API BindingConcept : public PropertyValue<T, mutex>
{
    using Base = PropertyValue<T, mutex>;
    DerivedBinding* getSelf()
    {
        return static_cast<DerivedBinding*>(this);
    }

protected:
    explicit BindingConcept(WriteBehavior behavior)
        : Base(behavior)
    {
    }

    T evaluateOverride() override
    {
        BindingScope scope(getSelf());
        getSelf()->clearTrackables();
        return getSelf()->evaluateBinding();
    }

    bool setOverride(const T&) override
    {
        COMP_ASSERT(false);
        return false;
    }

    void removeSelf() override
    {
        getSelf()->getProperty()->removePropertyValue(*getSelf());
    }
};

} // comp

#endif // COMP_BINDING_HPP
