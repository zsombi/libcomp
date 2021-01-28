#ifndef COMP_ZERO_SAFE_VECTOR_HPP
#define COMP_ZERO_SAFE_VECTOR_HPP

#include <vector>

#include <comp/wrap/algorithm.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/optional.hpp>
#include <comp/wrap/type_traits.hpp>

namespace comp
{

namespace detail
{

template <typename T>
struct NullCheck
{
    bool operator()(const T& value)
    {
        return !value;
    }
};

template <typename T>
struct ZeroSet
{
    void operator()(T& v)
    {
        if constexpr (is_shared_ptr_v<T> || is_weak_ptr_v<T>)
        {
            v.reset();
        }
        if constexpr (is_pointer_v<T>)
        {
            v = nullptr;
        }
        if constexpr (is_integral_v<T>)
        {
            v = 0;
        }
        if constexpr (is_class_v<T>)
        {
            v = T();
        }
    }
};

} // detail

template <typename Type,
          typename NullChecker = detail::NullCheck<Type>,
          typename Invalidator = detail::ZeroSet<Type>>
class COMP_TEMPLATE_API ZeroSafeContainer
{
    using Self = ZeroSafeContainer<Type, NullChecker, Invalidator>;

public:
    using ContainerType = vector<Type>;

    explicit ZeroSafeContainer() = default;

    ZeroSafeContainer(Self&& other)
    {
        *this = forward<Self>(other);
    }

    Self& operator=(Self&& other)
    {
        swap(m_container, other.m_container);
        swap(m_refCount, other.m_refCount);
        swap(m_dirtyCount, other.m_dirtyCount);
        return *this;
    }
    ~ZeroSafeContainer()
    {
        clear();
    }

    operator ContainerType() const
    {
        return m_container;
    }

    size_t lockCount() const
    {
        return m_refCount;
    }

    void lock()
    {
        ++m_refCount;
    }

    void unlock()
    {
        if (--m_refCount <= 0 && m_dirtyCount > 0u)
        {
            if (m_dirtyCount == m_container.size())
            {
                m_container.clear();
            }
            else
            {
                erase_if(m_container, NullChecker());
            }
            m_dirtyCount = 0;
        }
    }

    bool empty() const
    {
        return !size();
    }

    size_t size() const
    {
        return m_container.size() - m_dirtyCount;
    }

    Type& back()
    {
        return m_container.back();
    }

    void push_back(Type value)
    {
        lock_guard guard(*this);
        m_container.push_back(value);
    }

    template <typename Predicate>
    bool push_back_if(Type value, Predicate predicate)
    {
        lock_guard guard(*this);
        auto idx = find_if(m_container.begin(), m_container.end(), predicate);
        if (idx != m_container.end())
        {
            return false;
        }
        push_back(forward<Type>(value));
        return true;
    }

    void emplace_back(Type&& value)
    {
        lock_guard guard(*this);
        m_container.emplace_back(forward<Type>(value));
    }

    template <typename Predicate>
    bool emplace_back_if(Type&& value, Predicate predicate)
    {
        lock_guard guard(*this);
        auto idx = find_if(m_container.begin(), m_container.end(), predicate);
        if (idx != m_container.end())
        {
            return false;
        }
        emplace_back(forward<Type>(value));
        return true;
    }

    void erase(typename ContainerType::iterator it)
    {
        lock_guard guard(*this);
        Invalidator()(*it);
        ++m_dirtyCount;
    }

    void erase(typename ContainerType::iterator first, typename ContainerType::iterator last)
    {
        lock_guard guard(*this);
        for_each(first, last, Invalidator());
        m_dirtyCount += distance(first, last);
    }

    void clear()
    {
        erase(m_container.begin(), m_container.end());
    }

private:
    ContainerType m_container;
    size_t m_refCount = 0;
    size_t m_dirtyCount = 0;

    COMP_DISABLE_COPY(ZeroSafeContainer)

    template <typename T, typename C, typename I, typename F>
    friend void for_each(ZeroSafeContainer<T, C, I>&, F);

    template <typename T, typename C, typename I, typename VT>
    friend optional<T> find(ZeroSafeContainer<T, C, I>&, VT);

    template <typename T, typename C, typename I, typename F>
    friend optional<T> find_if(ZeroSafeContainer<T, C, I>&, F);

    template <typename T, typename C, typename I, typename F>
    friend optional<T> reverse_find_if(ZeroSafeContainer<T, C, I>&, F);

    template <typename T, typename C, typename I, typename VT>
    friend void erase(ZeroSafeContainer<T, C, I>&, const VT&);

    template <typename T, typename C, typename I, typename F>
    friend void erase_if(ZeroSafeContainer<T, C, I>&, F);
};


/// Non-member functions of ZeroSafeContainer

template <typename Type, typename IsNull, typename Invalidate, typename Function>
void for_each(ZeroSafeContainer<Type, IsNull, Invalidate>& shv, Function function)
{
    lock_guard ref(shv);
    auto end = shv.m_container.end();

    auto predicate = [&function](auto& element)
    {
        if (!IsNull()(element))
        {
            return;
        }
        function(element);
    };
    for_each(shv.m_container.begin(), end, predicate);
}

template <typename Type, typename IsNull, typename Invalidate, typename VType>
optional<Type> find(ZeroSafeContainer<Type, IsNull, Invalidate>& shv, VType value)
{
    lock_guard lock(shv);
    auto end = shv.m_container.end();
    auto it = find(shv.m_container.begin(), end, value);
    if (it != shv.m_container.end())
    {
        return make_optional(*it);
    }
    return nullopt;
}

template <typename Type, typename IsNull, typename Invalidate, typename Function>
optional<Type> find_if(ZeroSafeContainer<Type, IsNull, Invalidate>& shv, Function function)
{
    lock_guard ref(shv);
    auto end = shv.m_container.end();
    auto predicate = [&function](auto& element)
    {
        if (!IsNull()(element) && function(element))
        {
            return true;
        }
        return false;
    };
    auto it = find_if(shv.m_container.begin(), end, predicate);
    if (it != shv.m_container.end())
    {
        return make_optional(*it);
    }
    return nullopt;
}

template <typename Type, typename IsNull, typename Invalidate, typename Function>
optional<Type> reverse_find_if(ZeroSafeContainer<Type, IsNull, Invalidate>& shv, Function function)
{
    lock_guard ref(shv);
    auto end = shv.m_container.rend();
    auto predicate = [&function](auto& element)
    {
        if (!IsNull()(element) && function(element))
        {
            return true;
        }
        return false;
    };
    auto it = find_if(shv.m_container.rbegin(), end, predicate);
    if (it != shv.m_container.rend())
    {
        return make_optional(*it);
    }
    return nullopt;
}

template <typename Type, typename IsNull, typename Invalidate, typename VType>
void erase(ZeroSafeContainer<Type, IsNull, Invalidate>& shv, const VType& value)
{
    lock_guard lock(shv);
    auto begin = shv.m_container.begin();
    auto end = shv.m_container.end();
    shv.erase(remove(begin, end, value), end);
}

template <typename Type, typename IsNull, typename Invalidate, typename Function>
void erase_if(ZeroSafeContainer<Type, IsNull, Invalidate>& shv, Function function)
{
    lock_guard lock(shv);
    auto begin = shv.m_container.begin();
    auto end = shv.m_container.end();
    auto predicate = [&function](auto& element)
    {
        if (!IsNull()(element) && function(element))
        {
            return true;
        }
        return false;
    };
    shv.erase(remove_if(begin, end, predicate), end);
}

} // comp

#endif // COMP_ZERO_SAFE_VECTOR_HPP
