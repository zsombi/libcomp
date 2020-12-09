#ifndef EXTRAS_HPP
#define EXTRAS_HPP

#include <algorithm>
#include <vector>

namespace utils
{

/// Vector utility, loops a \a predicate through a \a vector.
template <typename Type, typename Allocator, typename Predicate>
void for_each(std::vector<Type, Allocator>& vector, const Predicate& predicate)
{
    std::for_each(vector.begin(), vector.end(), predicate);
}

/// Vector utility, removes the occurences of \a value from a \a vector.
template <typename Type, typename Allocator, typename VType>
void erase(std::vector<Type, Allocator>& vector, const VType& value)
{
    vector.erase(std::remove(vector.begin(), vector.end(), value), vector.end());
}

/// Vector utility, removes the occurences for which the predicate gives affirmative result.
template <typename Type, typename Allocator, typename Predicate>
void erase_if(std::vector<Type, Allocator>& v, const Predicate& predicate)
{
    v.erase(std::remove_if(v.begin(), v.end(), predicate), v.end());
}

}

#endif // EXTRAS_HPP
