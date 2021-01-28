#ifndef COMP_PROPERTY_CONCEPT_HPP
#define COMP_PROPERTY_CONCEPT_HPP

#include <comp/config.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/function_traits.hpp>
#include <comp/concept/zero_safe_container.hpp>
#include <comp/signal.hpp>

namespace comp
{

/// The enum defines the status of a property value.
enum class PropertyValueState
{
    /// The property value is detached.
    Detached,
    /// The property value is attaching.
    Attaching,
    /// The property value is detaching.
    Detaching,
    /// The property value is attached, and it is the active property value.
    Active,
    /// The property value is attached and inactive.
    Inactive
};

/// The enum defines the property value write behavior.
enum class WriteBehavior
{
    /// The property value is kept when the property setter is called.
    Keep,
    /// The property value is discarded when the property setter is called.
    Discard
};

namespace core
{

class Property;
class COMP_API Value : public Lockable<mutex>, private ConnectionTracker, public enable_shared_from_this<Value>
{
    friend class Property;

public:
    virtual ~Value() = default;
    virtual void removeSelf();

    void trackSource(Property& source);
    void untrackSources();
    void reset()
    {
        untrackSources();
        clearTrackables();
    }

    /// Returns the status of the property value.
    /// \return The status of the property value.
    PropertyValueState getState() const
    {
        return m_state;
    }

    /// Returns the property to which this property value is attached.
    Property* getProperty() const
    {
        return m_target;
    }

protected:

    virtual void onStateChanged(PropertyValueState /*state*/)
    {
    }

    void setState(PropertyValueState state);

    /// The source properties of this property value provider.
    vector<Property*> m_bindingSources;

    /// The target property to which the property value is attached.
    Property* m_target = nullptr;
    /// The state of a property value.
    PropertyValueState m_state = PropertyValueState::Detached;

};

class COMP_API Property : public Lockable<mutex>
{
    friend class Value;

public:
    /// The changed signal emits when the value of the property is changed. The signal is also
    /// emitted when the active property value is changed.
    comp::Signal<void()> changed;

    virtual ~Property()
    {
        notifyDeleted();
    }

    virtual void removePropertyValue(Value& /*value*/)
    {
    }

protected:
    virtual void notifyDeleted()
    {
        while (!m_connectedPropertyValues.empty())
        {
            auto value = m_connectedPropertyValues.back();
            m_connectedPropertyValues.pop_back();
            erase(value->m_bindingSources, this);
            value->removeSelf();
        }
    }

    vector<shared_ptr<Value>> m_connectedPropertyValues;
};

void Value::setState(PropertyValueState state)
{
    m_state = state;
    onStateChanged(m_state);
}

void Value::trackSource(Property& source)
{
    track(source.changed.connect(m_target->changed));
    source.m_connectedPropertyValues.push_back(shared_from_this());
    m_bindingSources.push_back(&source);
}

void Value::untrackSources()
{
    while (!m_bindingSources.empty())
    {
        auto* source = m_bindingSources.back();
        m_bindingSources.pop_back();
        erase(source->m_connectedPropertyValues, shared_from_this());
    }
}

void Value::removeSelf()
{
    reset();
}

} // comp::core

class BindingScope;

template <typename T>
class PropertyConcept;

typedef Signal<void()> ChangeSignalType;

/// The PropertyValue template defines the interface of property values. Property values are interfaces
/// that provide the actual value of a property. A property may have several property value providers, of
/// which only one is active.
template <typename T>
class COMP_TEMPLATE_API PropertyValue : public core::Value
{
public:
    using DataType = T;
    using PropertyType = core::Property;

    /// The behavior of the property value when the property setter is invoked.
    const WriteBehavior writeBehavior = WriteBehavior::Discard;

    /// Destructor.
    virtual ~PropertyValue() = default;

    /// Evaluates the property value, and returns the evaluated value, which represents the value of the property.
    /// \return The value of the property.
    DataType evaluate();

    /// Stores a value in the property value.
    /// \param value The value to store in the property value.
    /// \return If the property value is changed, returns \e true, otherwise \e false.
    void set(const DataType& value);

    /// Checks if the property value is the active property value.
    bool isActive() const;

    /// Activates a property value. The property value must be attached to a property.
    void activate();

    /// Deactivates a property value. The property value must be attached and active.
    void deactivate();

    /// Attaches the property value to a \a property. The property value must not be attached to any property.
    void attach(PropertyType& property);

    /// Detaches the property value from a property.
    void detach();

protected:
    /// Constructor.
    explicit PropertyValue(WriteBehavior writeBehavior)
        : writeBehavior(writeBehavior)
    {
    }

    /// \name Overridables
    /// \{

    /// Ovberridable to evaluate a property value. Returns the result of the evaluation.
    /// \return The result of the evaluation.
    virtual DataType evaluateOverride() = 0;

    /// Value setter overridable.
    virtual bool setOverride(const DataType&) = 0;

    /// Methoid called when the state is changed.
    virtual void onStateChanged(PropertyValueState state);
    /// \}

private:

    /// Reentrance guard for evaluate.
    FlagGuard m_guardEvaluate;
};

template <typename T>
using PropertyValuePtr = shared_ptr<PropertyValue<T>>;
template <typename T>
using PropertyValueWeakPtr = weak_ptr<PropertyValue<T>>;

class COMP_API BindingScope final
{
public:
    static inline core::Value* current = nullptr;

    template <typename T>
    BindingScope(PropertyValue<T>* valueProvider)
        : previousValue(current)
    {
        current = valueProvider;
    }

    ~BindingScope()
    {
        current = previousValue;
    }

private:
    core::Value* previousValue = nullptr;

    static inline ConnectionTracker* targetTracker = nullptr;
};


/// The StateConcept template defines the core functionality of state properties. State properties are read-only
/// properties. State properties have a single property value provider. To set the value of the property, use the
/// set method of the property value provider you pass to the constructor.
template <typename T>
class COMP_TEMPLATE_API StateConcept : public core::Property
{
    using Base = core::Property;

protected:
    using ValuePtr = PropertyValuePtr<T>;
    /// Constructor.
    explicit StateConcept(ValuePtr propertyValue);

    /// The property value provider.
    ValuePtr m_value;
};

/// The PropertyConcept template defines the core functionality of the properties. Provides property value
/// provider management, as well as notification of the property value changes.
/// A property must have at least one property value provider with WriteBehavior::Keep.
template <typename T>
class COMP_TEMPLATE_API PropertyConcept : public core::Property
{
    using Base = core::Property;

public:
    using ValuePtr = PropertyValuePtr<T>;

    /// Adds a property value to a property. The property value becomes the active property value.
    /// \param propertyValue The property value to add.
    void addPropertyValue(ValuePtr propertyValue);

    /// Removes a property value from a property. If the property value is the active property value,
    /// the last added property value becomes the active property value for the property.
    /// \param value The property value to remove.
    void removePropertyValue(core::Value& value);

    template <class Expression>
    enable_if_t<is_function_v<Expression> || function_traits<Expression>::type == Functor, PropertyValuePtr<T>>
    bind(Expression expression);

protected:
    /// Constructor.
    explicit PropertyConcept(ValuePtr defaultValue);

    /// Removes the discardable property values. Makes the last added value provider that is kept as
    /// the active value provider.
    void discardValues();

    /// Returns the active property value of the property.
    ValuePtr getActiveValue() const;

    struct VPInvalidator
    {
        void operator()(ValuePtr& propertyValue)
        {
            if (!propertyValue)
            {
                return;
            }
            propertyValue->detach();
            propertyValue.reset();
        }
    };

    ZeroSafeContainer<ValuePtr, detail::NullCheck<ValuePtr>, VPInvalidator> m_vp;
    PropertyValueWeakPtr<T> m_active;
};

} // namespace comp

#endif // COMP_PROPERTY_CONCEPT_HPP
