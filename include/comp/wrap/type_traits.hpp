#ifndef COMP_TYPE_TRAITS_HPP
#define COMP_TYPE_TRAITS_HPP

#include <type_traits>

namespace comp
{

using std::false_type;
using std::true_type;
using std::declval;
using std::decay;
using std::decay_t;
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
using std::is_member_function_pointer;
using std::is_member_function_pointer_v;
using std::is_pointer;
using std::is_pointer_v;
using std::remove_pointer;
using std::remove_pointer_t;

} // namespace traits

#endif // COMP_TYPE_TRAITS_HPP
