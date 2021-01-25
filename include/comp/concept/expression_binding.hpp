#ifndef COMP_EXPRESSION_BINDING_HPP
#define COMP_EXPRESSION_BINDING_HPP

#include <comp/concept/property.hpp>
#include <comp/wrap/memory.hpp>

namespace comp
{

template <typename T, typename LockType, class Expression>
class COMP_TEMPLATE_API ExpressionBinding : public PropertyValue<T, LockType>
{
    using Base = PropertyValue<T, LockType>;

public:
    static auto create(Expression expression)
    {
        auto binding = make_shared<Base>(new ExpressionBinding(expression));
        return binding;
    }

protected:
    explicit ExpressionBinding(Expression expression)
        : Base(WriteBehavior::Discard)
        , m_expression(expression)
    {
    }

    T evaluateOverride() override
    {
        BindingScope scope(this);
        return m_expression();
    }

    bool setOverride(const T&) override
    {
        COMP_ASSERT(false);
        return false;
    }

    void removeSelf() override
    {
        this->getProperty()->removePropertyValue(*this);
    }

    Expression m_expression;
};

} // comp

#endif // COMP_EXPRESSION_BINDING_HPP
