#ifndef COMP_PROPERTY_IMPL_HPP
#define COMP_PROPERTY_IMPL_HPP

#include <comp/concept/property.hpp>
#include <comp/wrap/algorithm.hpp>
#include <comp/wrap/type_traits.hpp>
#include <comp/wrap/utility.hpp>

namespace comp
{

template <typename T>
T PropertyValue<T>::evaluate()
{
    ScopeValue guard(m_status, PropertyValueStatus::Evaluating);
    return evaluateOverride();
}

template <typename T>
bool PropertyValue<T>::set(const DataType& value)
{
    lock_guard lock(*this);
    return setOverride(value);
}

template <typename T>
void PropertyValue<T>::swap(PropertyValue& other)
{
    swapOverride(other);
}

template <typename T>
bool PropertyValue<T>::isActive() const
{
    COMP_ASSERT(m_status == PropertyValueStatus::Active || m_status == PropertyValueStatus::Inactive || m_status == PropertyValueStatus::Evaluating);
    return m_status == PropertyValueStatus::Active || m_status == PropertyValueStatus::Evaluating;
}

template <typename T>
void PropertyValue<T>::activate()
{
    COMP_ASSERT(m_status == PropertyValueStatus::Active || m_status == PropertyValueStatus::Inactive);
    if (m_status == PropertyValueStatus::Inactive)
    {
        m_status = PropertyValueStatus::Active;
        m_target->changed(evaluate());
    }
}

template <typename T>
void PropertyValue<T>::deactivate()
{
    COMP_ASSERT(m_status == PropertyValueStatus::Active);
    m_status = PropertyValueStatus::Inactive;
}

template <typename T>
void PropertyValue<T>::attach(PropertyCore<T>& property)
{
    COMP_ASSERT(m_status == PropertyValueStatus::Detached);
    m_status = PropertyValueStatus::Attaching;
    m_target = &property;
    m_status = PropertyValueStatus::Inactive;
}

template <typename T>
void PropertyValue<T>::detach()
{
    COMP_ASSERT(m_status != PropertyValueStatus::Detached || m_status != PropertyValueStatus::Detaching);
    m_status = PropertyValueStatus::Detaching;
    m_status = PropertyValueStatus::Detached;
}

/***********
 *
 */
template <typename T>
PropertyCore<T>::PropertyCore(ValuePtr defaultValue)
{
    addPropertyValue(defaultValue);
}

template <typename T>
void PropertyCore<T>::addPropertyValue(ValuePtr propertyValue)
{
    m_vp.push_back(propertyValue);
    m_vp.back()->attach(*this);

    auto previousActive = m_active.lock();
    if (previousActive)
    {
        previousActive->deactivate();
    }

    m_active = m_vp.back();
    propertyValue->activate();
}

template <typename T>
void PropertyCore<T>::removePropertyValue(PropertyValue<T>& propertyValue)
{
    auto choseNewActive = propertyValue.isActive();
    auto detach = [&propertyValue](auto vp)
    {
        if (vp.get() == &propertyValue)
        {
            vp->detach();
            return true;
        }
        return false;
    };
    find_if(m_vp, detach);

    if (choseNewActive)
    {
        auto last = m_vp.back();
        last->activate();
        m_active = last;
    }
}

template <typename T>
void PropertyCore<T>::discard()
{
    auto checkNewActive = false;
    auto predicate = [&checkNewActive](auto& vp)
    {
        if (vp->writeBehavior == WriteBehavior::Discard)
        {
            checkNewActive |= vp->isActive();
            if (vp->isActive())
            {
                vp->deactivate();
            }
            return true;
        }
        return false;
    };
    erase_if(m_vp, predicate);

    if (checkNewActive)
    {
        m_active = m_vp.back();
        m_active.lock()->activate();
    }
}

template <typename T>
typename PropertyCore<T>::ValuePtr PropertyCore<T>::getActiveValue() const
{
    return m_active.lock();
}

} // comp

#endif // PROPERTY_IMPL_HPP
