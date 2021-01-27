#ifndef COMP_PROPERTY_IMPL_HPP
#define COMP_PROPERTY_IMPL_HPP

#include <comp/concept/property.hpp>
#include <comp/utility/scope_value.hpp>
#include <comp/wrap/algorithm.hpp>
#include <comp/wrap/type_traits.hpp>
#include <comp/wrap/utility.hpp>

#include <comp/concept/expression_binding.hpp>

namespace comp
{

template <typename T, typename LockType>
void PropertyValue<T, LockType>::setState(PropertyValueState state)
{
    m_state = state;
    onStateChanged(m_state);
}

template <typename T, typename LockType>
T PropertyValue<T, LockType>::evaluate()
{
    lock_guard lock(*this);
    // Do not allow the property to add or remove value providers till this property value is evaluating.
    lock_guard superLock(*m_target);

    if (m_guardEvaluate.isLocked())
    {
        return T();
    }
    lock_guard guard(m_guardEvaluate);

    if (BindingScope::current)
    {
        BindingScope::trackProperty(*m_target);
    }

    return evaluateOverride();
}

template <typename T, typename LockType>
void PropertyValue<T, LockType>::set(const DataType& value)
{
    lock_guard lock(*this);
    if (setOverride(value) && isActive())
    {
        m_target->changed();
    }
}

template <typename T, typename LockType>
bool PropertyValue<T, LockType>::isActive() const
{
    COMP_ASSERT(m_state == PropertyValueState::Active || m_state == PropertyValueState::Inactive);
    return m_state == PropertyValueState::Active;
}

template <typename T, typename LockType>
void PropertyValue<T, LockType>::activate()
{
    COMP_ASSERT(m_state == PropertyValueState::Active || m_state == PropertyValueState::Inactive);
    if (m_state == PropertyValueState::Inactive)
    {
        setState(PropertyValueState::Active);
        m_target->changed();
    }
}

template <typename T, typename LockType>
void PropertyValue<T, LockType>::deactivate()
{
    COMP_ASSERT(m_state == PropertyValueState::Active);
    setState(PropertyValueState::Inactive);
}

template <typename T, typename LockType>
void PropertyValue<T, LockType>::attach(Core& property)
{
    COMP_ASSERT(m_state == PropertyValueState::Detached);
    setState(PropertyValueState::Attaching);
    m_target = &property;
    setState(PropertyValueState::Inactive);
}

template <typename T, typename LockType>
void PropertyValue<T, LockType>::detach()
{
    COMP_ASSERT(m_state != PropertyValueState::Detached || m_state != PropertyValueState::Detaching);
    setState(PropertyValueState::Detaching);
    m_target = nullptr;
    setState(PropertyValueState::Detached);
}

template <typename T, typename LockType>
void PropertyValue<T, LockType>::onStateChanged(PropertyValueState state)
{
    switch (state)
    {
        case PropertyValueState::Active:
        {
            evaluate();
            break;
        }
        case PropertyValueState::Inactive:
        case PropertyValueState::Detaching:
        {
            clearTrackables();
            break;
        }
        default:
        {
            break;
        }
    }
}

/***********
 *
 */
template <typename T, typename LockType>
StateConcept<T, LockType>::StateConcept(typename Base::ValuePtr propertyValue)
    : m_value(propertyValue)
{
    m_value->attach(*this);
    m_value->activate();
}


template <typename T, typename LockType>
PropertyConcept<T, LockType>::PropertyConcept(typename Base::ValuePtr propertyValue)
{
    addPropertyValue(propertyValue);
}

template <typename T, typename LockType>
void PropertyConcept<T, LockType>::addPropertyValue(typename Base::ValuePtr propertyValue)
{
    lock_guard lock(*this);

    m_vp.push_back(propertyValue);
    m_vp.back()->attach(*this);

    auto previousActive = m_active.lock();
    if (previousActive)
    {
        previousActive->deactivate();
    }

    m_active = m_vp.back();

    relock_guard relock(*this);
    propertyValue->activate();
}

template <typename T, typename LockType>
void PropertyConcept<T, LockType>::removePropertyValue(core::Value& value)
{
    lock_guard lock(*this);

    auto propertyValue = dynamic_cast<PropertyValue<T, LockType>*>(&value);

    auto choseNewActive = propertyValue->isActive();
    auto detach = [propertyValue](auto vp)
    {
        if (vp.get() == propertyValue)
        {
            vp->detach();
            return true;
        }
        return false;
    };
    erase_if(m_vp, detach);

    if (choseNewActive)
    {
        auto last = m_vp.back();
        m_active = last;
        relock_guard relock(*this);
        last->activate();
    }
}

template <typename T, typename LockType>
void PropertyConcept<T, LockType>::discardValues()
{
    lock_guard lock(*this);

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
        relock_guard relock(*this);
        m_active.lock()->activate();
    }
}

template <typename T, typename LockType>
typename PropertyCore<T, LockType>::ValuePtr PropertyConcept<T, LockType>::getActiveValue() const
{
    return m_active.lock();
}

template <typename T, typename LockType>
template <class Expression>
enable_if_t<is_function_v<Expression> || function_traits<Expression>::type == Functor, PropertyValuePtr<T, LockType>>
PropertyConcept<T, LockType>::bind(Expression expression)
{
    auto binding = ExpressionBinding<T, Expression>::create(expression);
    addPropertyValue(binding);
    return binding;
}

} // comp

#endif // PROPERTY_IMPL_HPP
