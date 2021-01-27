#ifndef COMP_EXPRESSION_BINDING_HPP
#define COMP_EXPRESSION_BINDING_HPP

#include <comp/concept/binding.hpp>
#include <comp/wrap/memory.hpp>

namespace comp
{

template <typename T, class Expression>
class COMP_TEMPLATE_API ExpressionBinding : public BindingConcept<ExpressionBinding<T, Expression>, T>
{
    using Base = BindingConcept<ExpressionBinding<T, Expression>, T>;

public:
    using ExpressionType = T;
    static auto create(Expression expression)
    {
        auto binding = make_shared<Base>(new ExpressionBinding(expression));
        return binding;
    }

    T evaluateBinding()
    {
        return m_expression();
    }

protected:
    explicit ExpressionBinding(Expression expression)
        : Base(WriteBehavior::Discard)
        , m_expression(expression)
    {
    }

    Expression m_expression;
};

} // comp

#endif // COMP_EXPRESSION_BINDING_HPP
