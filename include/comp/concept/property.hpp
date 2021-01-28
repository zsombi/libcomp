#ifndef COMP_PROPERTY_CONCEPT_HPP
#define COMP_PROPERTY_CONCEPT_HPP

#include <comp/config.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/function_traits.hpp>
#include <comp/concept/zero_safe_container.hpp>
#include <comp/signal.hpp>

namespace comp
{

namespace core
{

class Property;
class COMP_API Value : public ConnectionTracker, public enable_shared_from_this<Value>
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

protected:
    vector<Property*> m_sourceProperties;
};

class COMP_API Property
{
    friend class Value;

public:
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
            erase(value->m_sourceProperties, this);
            value->removeSelf();
        }
    }

    vector<shared_ptr<Value>> m_connectedPropertyValues;
};

void Value::trackSource(Property& source)
{
    source.m_connectedPropertyValues.push_back(shared_from_this());
    m_sourceProperties.push_back(&source);
}

void Value::untrackSources()
{
    while (!m_sourceProperties.empty())
    {
        auto* source = m_sourceProperties.back();
        m_sourceProperties.pop_back();
        erase(source->m_connectedPropertyValues, shared_from_this());
    }
}

void Value::removeSelf()
{
    untrackSources();
}

} // comp::core

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

class BindingScope;

template <typename T, typename LockType>
class PropertyCore;

typedef Signal<void()> ChangeSignalType;

/// The PropertyValue template defines the interface of property values. Property values are interfaces
/// that provide the actual value of a property. A property may have several property value providers, of
/// which only one is active.
template <typename T, typename LockType>
class COMP_TEMPLATE_API PropertyValue : public Lockable<LockType>, public core::Value
{
public:
    using DataType = T;
    using Core = PropertyCore<T, LockType>;

    /// The behavior of the property value when the property setter is invoked.
    const WriteBehavior writeBehavior = WriteBehavior::Discard;

    /// Destructor.
    virtual ~PropertyValue() = default;

    /// Returns the status of the property value.
    /// \return The status of the property value.
    PropertyValueState getState() const
    {
        return m_state;
    }

    Core* getProperty() const
    {
        return m_target;
    }

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
    void attach(Core& property);

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

    /// The target property to which the property value is attached.
    Core* m_target = nullptr;
    /// The state of a property value.
    PropertyValueState m_state = PropertyValueState::Detached;

private:
    void setState(PropertyValueState state);

    /// Reentrance guard for evaluate.
    FlagGuard m_guardEvaluate;
};

template <typename T, typename LockType>
using PropertyValuePtr = shared_ptr<PropertyValue<T, LockType>>;
template <typename T, typename LockType>
using PropertyValueWeakPtr = weak_ptr<PropertyValue<T, LockType>>;

class COMP_API BindingScope final
{
public:
    static inline core::Value* current = nullptr;

    template <typename T, typename LockType>
    BindingScope(PropertyValue<T, LockType>* valueProvider)
        : previousValue(current)
        , previousChangeSignal(targetChangeSignal)
    {
        current = valueProvider;
        targetChangeSignal = &valueProvider->getProperty()->changed;
    }

    ~BindingScope()
    {
        current = previousValue;
        targetChangeSignal = previousChangeSignal;
    }

    template <typename T, typename LockType>
    static void trackProperty(PropertyCore<T, LockType>& source)
    {
        current->track(source.changed.connect(*targetChangeSignal));
        current->trackSource(source);
    }

private:
    core::Value* previousValue = nullptr;
    ChangeSignalType* previousChangeSignal = nullptr;

    static inline ChangeSignalType* targetChangeSignal = nullptr;
    static inline ConnectionTracker* targetTracker = nullptr;
};


template <typename T, typename LockType>
class COMP_TEMPLATE_API PropertyCore : public Lockable<LockType>, public core::Property
{
    template <typename, typename>
    friend class PropertyValue;
    friend class BindingScope;

public:
    using ValuePtr = PropertyValuePtr<T, LockType>;

    /// The changed signal emits when the value of the property is changed. The signal is also
    /// emitted when the active property value is changed.
    ChangeSignalType changed;

protected:
    explicit PropertyCore() = default;
};

/// The StateConcept template defines the core functionality of state properties. State properties are read-only
/// properties. State properties have a single property value provider. To set the value of the property, use the
/// set method of the property value provider you pass to the constructor.
template <typename T, typename LockType>
class COMP_TEMPLATE_API StateConcept : public PropertyCore<T, LockType>
{
    using Base = PropertyCore<T, LockType>;

protected:
    /// Constructor.
    explicit StateConcept(typename Base::ValuePtr propertyValue);

    /// The property value provider.
    typename Base::ValuePtr m_value;
};

/// The PropertyConcept template defines the core functionality of the properties. Provides property value
/// provider management, as well as notification of the property value changes.
/// A property must have at least one property value provider with WriteBehavior::Keep.
template <typename T, typename LockType>
class COMP_TEMPLATE_API PropertyConcept : public PropertyCore<T, LockType>
{
    using Base = PropertyCore<T, LockType>;

public:
    /// Adds a property value to a property. The property value becomes the active property value.
    /// \param propertyValue The property value to add.
    void addPropertyValue(typename Base::ValuePtr propertyValue);

    /// Removes a property value from a property. If the property value is the active property value,
    /// the last added property value becomes the active property value for the property.
    /// \param value The property value to remove.
    void removePropertyValue(core::Value& value);

    template <class Expression>
    enable_if_t<is_function_v<Expression> || function_traits<Expression>::type == Functor, PropertyValuePtr<T, LockType>>
    bind(Expression expression);

protected:
    /// Constructor.
    explicit PropertyConcept(typename Base::ValuePtr defaultValue);

    /// Removes the discardable property values. Makes the last added value provider that is kept as
    /// the active value provider.
    void discardValues();

    /// Returns the active property value of the property.
    typename Base::ValuePtr getActiveValue() const;

    struct VPInvalidator
    {
        void operator()(typename Base::ValuePtr& propertyValue)
        {
            if (!propertyValue)
            {
                return;
            }
            propertyValue->detach();
            propertyValue.reset();
        }
    };

    ZeroSafeContainer<typename Base::ValuePtr, detail::NullCheck<typename Base::ValuePtr>, VPInvalidator> m_vp;
    PropertyValueWeakPtr<T, LockType> m_active;
};

} // namespace comp

#endif // COMP_PROPERTY_CONCEPT_HPP
