#ifndef SYWU_EXTRAS_HPP
#define SYWU_EXTRAS_HPP

namespace sywu
{

#include <algorithm>

using std::for_each;
using std::find;
using std::find_if;
using std::remove;
using std::remove_if;

template <class Function, class... Arguments>
void for_each_arg(Function f, Arguments&&... args)
{
    (void)(int[]){(f(forward<Arguments>(args)), 0)...};
}

} // namespace sywu

#endif // SYWU_EXTRAS_HPP
