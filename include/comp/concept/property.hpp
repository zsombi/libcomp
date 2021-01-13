#ifndef COMP_PROPERTY_HPP
#define COMP_PROPERTY_HPP

#include <comp/config.hpp>
#include <comp/wrap/algorithm.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/signal.hpp>

namespace comp
{

template <typename T>
class COMP_TEMPLATE_API Property final : public Lockable<mutex>
{
public:
    using element_type = T;
    using changed_signal_type = Signal<void(T const&)>;

    changed_signal_type changed;

    Property() = default;
    Property(T const& defaultValue)
        : m_data(defaultValue)
    {
    }
    Property(Property const& other)
        : m_data(other.m_data)
    {
    }
    Property(Property&& other)
    {
        swap(other);
    }

    operator element_type() const
    {
        return m_data;
    }
    void operator=(T const& value)
    {
        if (value != m_data)
        {
            m_data = value;
            changed(m_data);
        }
    }
    Property& operator=(Property const& other)
    {
        if (other.m_data != m_data)
        {
            m_data = other.m_data;
            changed(m_data);
        }
        return *this;
    }
    Property& operator=(Property&& rhs)
    {
        Property(move(rhs)).swap(*this);
        changed(m_data);
        return *this;
    }

    void swap(Property& other)
    {
        comp::swap(other.m_data, m_data);
    }

private:

    element_type m_data = element_type();
};

} // namespace comp

#endif // COMP_PROPERTY_HPP
