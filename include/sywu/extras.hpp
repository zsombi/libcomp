#ifndef SYWU_EXTRAS_HPP
#define SYWU_EXTRAS_HPP

#include <algorithm>
#include <vector>

#ifdef DEBUG
#include <cassert>
#define SYWU_ASSERT(test)    if (!(test)) abort()
#else
#define SYWU_ASSERT(test)    (void)(test)
#endif

namespace utils
{

/// Vector utility, loops a \a predicate through a \a vector.
template <typename Type, typename Allocator, typename Predicate>
void for_each(std::vector<Type, Allocator>& vector, const Predicate& predicate)
{
    std::for_each(vector.begin(), vector.end(), predicate);
}
template <typename Type, typename Allocator, typename Predicate>
void for_each(const std::vector<Type, Allocator>& vector, const Predicate& predicate)
{
    std::for_each(vector.begin(), vector.end(), predicate);
}

/// Vector utility, loops a \a predicate through a \a vector.
template <typename Type, typename Allocator, typename Predicate>
decltype(auto) find_if(std::vector<Type, Allocator>& vector, const Predicate& predicate)
{
    return std::find_if(vector.begin(), vector.end(), predicate);
}
template <typename Type, typename Allocator, typename Predicate>
decltype(auto) find_if(const std::vector<Type, Allocator>& vector, const Predicate& predicate)
{
    return std::find_if(vector.begin(), vector.end(), predicate);
}

/// Vector utility, removes the occurences of \a value from a \a vector.
template <typename Type, typename Allocator, typename VType>
void erase(std::vector<Type, Allocator>& vector, const VType& value)
{
    vector.erase(std::remove(vector.begin(), vector.end(), value), vector.end());
}

/// Vector utility, removes the first occurence of \a value from a \a vector.
template <typename Type, typename Allocator, typename VType>
void erase_first(std::vector<Type, Allocator>& vector, const VType& value)
{
    auto it = std::find(vector.begin(), vector.end(), value);
    if (it != vector.end())
    {
        vector.erase(it);
    }
}

/// Vector utility, removes the occurences for which the predicate gives affirmative result.
template <typename Type, typename Allocator, typename Predicate>
void erase_if(std::vector<Type, Allocator>& v, const Predicate& predicate)
{
    v.erase(std::remove_if(v.begin(), v.end(), predicate), v.end());
}

template <class Function, class... Arguments>
void for_each_arg(Function f, Arguments&&... args)
{
    (void)(int[]){(f(std::forward<Arguments>(args)), 0)...};
}

}

#endif // SYWU_EXTRAS_HPP
