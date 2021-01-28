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

    void removeSelf() override
    {
        Base::removeSelf();
        auto property = getSelf()->getProperty();
        if (property)
        {
            property->removePropertyValue(*getSelf());
        }
        else
        {
            std::puts("null property on removeSelf()");
        }
    }
};

} // comp

#endif // COMP_BINDING_HPP
