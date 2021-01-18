#ifndef COMP_SCOPE_VALUE_HPP
#define COMP_SCOPE_VALUE_HPP

namespace comp
{

/// The template flips a variable value to a temporary value for the lifetime of the class.
/// When the class is destroyed, it restores the value of the variable at the class creation
/// time.
template <typename T>
class ScopeValue
{
public:
    explicit ScopeValue(T& variable, T value)
        : m_variable(variable)
        , m_previousValue(m_variable)
    {
        m_variable = value;
    }
    ~ScopeValue()
    {
        m_variable = m_previousValue;
    }
private:
    T& m_variable;
    T m_previousValue;
};

} // comp

#endif // COMP_SCOPE_VALUE_HPP
