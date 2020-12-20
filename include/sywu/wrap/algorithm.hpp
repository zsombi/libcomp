#ifndef SYWU_ALGORITHM_HPP
#define SYWU_ALGORITHM_HPP

#include <algorithm>

namespace sywu
{

using std::for_each;
using std::find;
using std::find_if;
using std::remove;
using std::remove_if;

/// Template function to call a function \a f on an rgument pack. The function is expected to take a single
/// argument.
template <class Function, class... Arguments>
void for_each_arg(Function f, Arguments&&... args)
{
    (void)(int[]){(f(forward<Arguments>(args)), 0)...};
}

} // namespace sywu

#endif // SYWU_ALGORITHM_HPP
