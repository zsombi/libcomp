#ifndef SYWU_TYPE_TRAITS_HPP
#define SYWU_TYPE_TRAITS_HPP

#include <array>
#include <cstddef>
#include <tuple>
#include <vector>

namespace traits
{

template <typename T>
struct remove_cvref
{
    typedef std::remove_const_t<std::remove_reference_t<T>> type;
};
template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;


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

/// Tests a function for
template <typename Function>
struct is_lambda
{
    static constexpr bool value = function_traits<Function>::type == FunctionType::Functor;
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
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    struct test_arguments
    {
        static constexpr bool value = std::is_same_v<std::tuple<Args...>, std::tuple<TestArgs...>>;
    };
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
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    struct test_arguments
    {
        static constexpr bool value = std::is_same_v<std::tuple<Args...>, std::tuple<TestArgs...>>;
    };
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
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    struct test_arguments
    {
        static constexpr bool value = std::is_same_v<std::tuple<Args...>, std::tuple<TestArgs...>>;
    };
};

} // namespace traits

#endif // SYWU_TYPE_TRAITS_HPP
