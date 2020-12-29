#ifndef SYWU_MEMORY_HPP
#define SYWU_MEMORY_HPP

#include <memory>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/type_traits.hpp>

namespace sywu
{

using std::pointer_traits;

using std::unique_ptr;
using std::default_delete;
using std::shared_ptr;
using std::weak_ptr;

using std::make_unique;
using std::make_shared;

using std::dynamic_pointer_cast;
using std::static_pointer_cast;

using std::enable_shared_from_this;

using std::bad_weak_ptr;

/// \name Traits
/// \{

/// Detect unique_ptr
template <typename T, typename = void>
struct is_unique_ptr : false_type {};

template <typename T>
struct is_unique_ptr<T, enable_if_t<is_same_v<decay_t<T>, unique_ptr<typename decay_t<T>::element_type, typename decay_t<T>::deleter_type>>>> : true_type {};

template <typename T>
constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

/// Detect weak_ptr
template <typename T, typename = void>
struct is_weak_ptr : false_type {};

template <typename T>
struct is_weak_ptr<T, enable_if_t<is_same_v<decay_t<T>, weak_ptr<typename decay_t<T>::element_type>>>> : true_type {};

template <typename T>
constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

/// Detect shared_ptr
template <typename T, typename = void>
struct is_shared_ptr : false_type {};

template <typename T>
struct is_shared_ptr<T, enable_if_t<is_same_v<decay_t<T>, shared_ptr<typename decay_t<T>::element_type>>>> : true_type {};

template <typename T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

/// Detect if a type is derived from enable_shared_from_this<>
template <typename T, typename = void>
struct has_shared_from_this : false_type {};

template <typename T>
struct has_shared_from_this<T, void_t<decltype(declval<T>().shared_from_this())>> : true_type {};

template <typename T>
constexpr bool has_shared_from_this_v = has_shared_from_this<T>::value;

/// \}

/// std::make_shared initiates lots of control templates, creates loads of control blocks and deleters,
/// which all increase the code size. This template offers smaller code size.
template <class Base, class Derived, class... Arguments>
shared_ptr<Base> make_shared(Arguments&&... args)
{
    return shared_ptr<Base>(static_cast<Base*>(new Derived(forward<Arguments>(args)...)));
}

} // namespace sywu

#endif // SYWU_MEMORY_HPP
