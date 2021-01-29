#ifndef COMP_PROPERTY_HPP
#define COMP_PROPERTY_HPP

#include <comp/config.hpp>
#include <comp/concept/property_concept_impl.hpp>

namespace comp
{

/// The Property provides you property management in COMP.
template <typename T>
class COMP_TEMPLATE_API Property final : public PropertyConcept<T>
{
    using Base = PropertyConcept<T>;
    using ValueTypePtr = PropertyValuePtr<T>;

    class COMP_TEMPLATE_API Data : public PropertyValue<T>
    {
    public:
        explicit Data(T const& value)
            : PropertyValue<T>(WriteBehavior::Keep)
            , m_data(value)
        {
        }

    protected:
        T evaluateOverride() override
        {
            return m_data;
        }
        bool setOverride(const T& value) override
        {
            if (m_data != value)
            {
                m_data.store(value);
                return true;
            }
            return false;
        }

    private:
        atomic<T> m_data = T();
    };

    COMP_DISABLE_MOVE(Property);

public:
    using DataType = T;

    /// Creates a property with an optional initial \a value.
    /// \param value The initial value of the property.
    explicit Property(const DataType& initialValue = DataType())
        : Base(make_shared<Data>(initialValue))
    {
    }
    /// Creates a property with a custom property value.
    /// \param propertyValue The property value with which the property is created. The property value must
    /// have #WriteBehavior::Keep write behavior.
    explicit Property(ValueTypePtr propertyValue)
        : Base(propertyValue)
    {
        COMP_ASSERT(propertyValue->writeBehavior == WriteBehavior::Keep);
    }
    /// Creates a property using the value of an other property with the same type.
    explicit Property(Property const& other)
        : Property(other.operator DataType())
    {
    }

    /// Property getter.
    /// \return The current value of the property, returned by the active property value provider.
    operator DataType() const
    {
        return this->getActiveValue()->evaluate();
    }

    /// Property setter.
    /// \param value The value to set. The property discards all the property value providers with WriteBehavior::Discard
    /// write behavior, and sets the \a value to the new active property value provider.
    void operator=(DataType const& value)
    {
        this->discardValues();
        this->getActiveValue()->set(value);
    }

    /// Sets the value of a property from the value of an other property of the same type.
    /// \param other The source property providing the value to set.
    /// \return The reference to the property.
    Property& operator=(Property const& other)
    {
        operator=(other.operator DataType());
        return *this;
    }
};

template <typename T>
class COMP_TEMPLATE_API State : public StateConcept<T>
{
    using Base = StateConcept<T>;

public:
    using DataType = T;
    /// Constructor, creates a state using a property value.
    /// \param propertyValue The property value which provides the value of the property.
    explicit State(PropertyValuePtr<T> propertyValue)
        : Base(propertyValue)
    {
    }

    /// Property getter.
    /// \return The value of the property.
    operator DataType() const
    {
        return this->m_value->evaluate();
    }
};

} // comp

#endif // COMP_PROPERTY_HPP
