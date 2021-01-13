#ifndef COMP_FUNCTION_TRAITS_HPP
#define COMP_FUNCTION_TRAITS_HPP

#include <comp/wrap/tuple.hpp>
#include <comp/wrap/type_traits.hpp>

namespace comp
{

enum FunctionType
{
    /// Specifies an invalid function.
    Invalid = -1,
    /// The callable is a function.
    Function,
    /// The callable is a functor.
    Functor,
    /// The callable is a method.
    Method
};

/// Traits for functors and function objects.
template <typename Function>
struct function_traits : public function_traits<decltype(&Function::operator())>
{
    static constexpr int type = FunctionType::Functor;
};

/// Method traits.
template <class TObject, typename TRet, typename... Args>
struct function_traits<TRet(TObject::*)(Args...)>
{
    using object = TObject;
    using return_type = TRet;
    typedef TRet(TObject::*function_type)(Args...);

    static constexpr std::size_t arity = sizeof... (Args);
    static constexpr bool is_const = false;
    static constexpr int type = FunctionType::Method;

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    static constexpr bool is_same_args = is_same_v<tuple<Args...>, tuple<TestArgs...>>;
};

/// Const method traits.
template <class TObject, typename TRet, typename... Args>
struct function_traits<TRet(TObject::*)(Args...) const>
{
    using object = TObject;
    using return_type = TRet;
    typedef TRet(TObject::*function_type)(Args...) const;

    static constexpr std::size_t arity = sizeof... (Args);
    static constexpr bool is_const = true;
    static constexpr int type = FunctionType::Method;

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    static constexpr bool is_same_args = is_same_v<tuple<Args...>, tuple<TestArgs...>>;
};

/// Function and static member function traits.
template <typename TRet, typename... Args>
struct function_traits<TRet(*)(Args...)>
{
    using return_type = TRet;
    typedef TRet(*function_type)(Args...);

    static constexpr std::size_t arity = sizeof... (Args);
    static constexpr bool is_const = false;
    static constexpr int type = FunctionType::Function;

    template <std::size_t N>
    struct argument
    {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    static constexpr bool is_same_args = is_same_v<tuple<Args...>, tuple<TestArgs...>>;
};

} // namespace comp

#endif // COMP_FUNCTION_TRAITS_HPP
