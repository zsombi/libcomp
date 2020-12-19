#ifndef SYWU_TYPE_TRAITS_HPP
#define SYWU_TYPE_TRAITS_HPP

#include <sywu/wrap/tuple.hpp>
#include <sywu/wrap/utility.hpp>

namespace sywu
{

#include <type_traits>

using std::enable_if;
using std::enable_if_t;
using std::is_same;
using std::is_same_v;
using std::is_base_of;
using std::is_base_of_v;
using std::void_t;
using std::is_void;
using std::is_void_v;
using std::remove_const_t;
using std::remove_reference_t;
using std::false_type;
using std::true_type;
using std::is_member_function_pointer;
using std::is_member_function_pointer_v;
using std::is_pointer;
using std::is_pointer_v;
using std::remove_pointer;
using std::remove_pointer_t;
using std::pointer_traits;

template <typename T>
struct remove_cvref
{
    typedef remove_const_t<remove_reference_t<T>> type;
};
template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;


/// Detect std::weak_ptr
template <typename T, typename = void>
struct is_weak_ptr : false_type {};

template <typename T>
struct is_weak_ptr<T, void_t<decltype(declval<T>().expired()), decltype(declval<T>().lock()), decltype(declval<T>().reset())>> : true_type {};

template <typename T>
constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

/// Detect a shared_ptr
template <typename T, typename = void>
struct is_shared_ptr : false_type {};

template <typename T>
struct is_shared_ptr<T, void_t<decltype(declval<T>().operator*()), decltype(declval<T>().operator->())>> : true_type {};

template <typename T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// Macro to detect the existence of a signal in a class.
///
#define HAS_SIGNAL(Name)                                                                \
template <typename T, typename = void>                                                  \
struct has_signal_##Name : false_type {};                                               \
template <typename T>                                                                   \
struct has_signal_##Name<T, void_t<decltype((void)T::##Name, void())>> : true_type {}


/// \name Function traits
/// \{
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
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    struct test_arguments
    {
        static constexpr bool value = is_same_v<tuple<Args...>, tuple<TestArgs...>>;
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
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    struct test_arguments
    {
        static constexpr bool value = is_same_v<tuple<Args...>, tuple<TestArgs...>>;
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
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };

    template <typename... TestArgs>
    struct test_arguments
    {
        static constexpr bool value = is_same_v<tuple<Args...>, tuple<TestArgs...>>;
    };
};

/// \}

} // namespace sywu

#endif // SYWU_TYPE_TRAITS_HPP
