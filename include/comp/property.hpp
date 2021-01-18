#ifndef COMP_PROPERTY_HPP
#define COMP_PROPERTY_HPP

#include <comp/config.hpp>
#include <comp/concept/property_concept_impl.hpp>

namespace comp
{

namespace
{

template <typename T>
class COMP_TEMPLATE_API PropertyData : public PropertyValue<T>
{
    using Base = PropertyValue<T>;

public:
    explicit PropertyData(T const& value)
        : Base(WriteBehavior::Keep)
        , m_data(value)
    {
    }

protected:
    typename Base::DataType evaluateOverride() override
    {
        return m_data;
    }
    bool setOverride(const typename Base::DataType & value) override
    {
        if (m_data != value)
        {
            m_data.store(value);
            return true;
        }
        return false;
    }
    void swapOverride(PropertyValue<T>& other) override
    {
//        scoped_lock lock(*this, other);
        static_cast<PropertyData&>(other).m_data = m_data.exchange(static_cast<PropertyData&>(other).m_data);
    }

private:
    atomic<T> m_data = T();
};

}

/// The Property provides you property management in COMP.
template <typename T>
class COMP_TEMPLATE_API Property final : public PropertyConcept<T>
{
    using Base = PropertyConcept<T>;

    COMP_DISABLE_MOVE(Property);

public:
    using DataType = T;

    /// Creates a property with an optional initial \a value.
    /// \param value The initial value of the property.
    explicit Property(const DataType& initialValue = DataType())
        : Base(make_shared<PropertyData<T>>(initialValue))
    {
    }
    /// Creates a property with a custom property value.
    /// \param propertyValue The property value with which the property is created. The property value must
    /// have #WriteBehavior::Keep write behavior.
    explicit Property(PropertyValuePtr<T> propertyValue)
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
        this->discard();
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
