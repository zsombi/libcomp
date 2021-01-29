#ifndef COMP_BINDING_HPP
#define COMP_BINDING_HPP

#include <comp/config.hpp>
#include <comp/concept/property.hpp>

namespace comp
{

template <class DerivedBinding, typename T>
class COMP_TEMPLATE_API BindingConcept : public PropertyValue<T>
{
    using Base = PropertyValue<T>;
    DerivedBinding* getSelf()
    {
        return static_cast<DerivedBinding*>(this);
    }

protected:
    explicit BindingConcept(WriteBehavior behavior)
        : Base(behavior)
    {
    }

    T evaluateOverride() final
    {
        BindingScope scope(getSelf());
        getSelf()->reset();
        return getSelf()->evaluateBinding();
    }

    bool setOverride(const T&) final
    {
        COMP_ASSERT(false);
        return false;
    }
};

} // comp

#endif // COMP_BINDING_HPP
