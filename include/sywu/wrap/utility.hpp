#ifndef SYWU_UTILITY_HPP
#define SYWU_UTILITY_HPP

#include <utility>

namespace sywu
{

using std::forward;
using std::move;
using std::exchange;

/// Template function to call a function \a f on an rgument pack. The function is expected to take a single
/// argument.
template <class Function, class... Arguments>
void for_each_arg(Function f, Arguments&&... args)
{
    (void)(int[]){(f(forward<Arguments>(args)), 0)...};
}

} // namespace sywu

#endif // SYWU_UTILITY_HPP
