#ifndef COMP_PROPERTY_H
#define COMP_PROPERTY_H

#include <comp/config.hpp>
#include <comp/signal.hpp>

namespace comp
{

template <typename T>
class COMP_TEMPLATE_API Property
{
public:
    Signal<void(T const&)> changed;

    explicit Property(const T& defaultValue = T())
    {
        COMP_UNUSED(defaultValue);
    }

    operator T() const
    {
        return T();
    }

    void operator=(const T& value)
    {
        COMP_UNUSED(value);
        changed(value);
    }
};

}

#endif
