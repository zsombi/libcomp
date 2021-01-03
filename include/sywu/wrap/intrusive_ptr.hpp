#ifndef SYWU_INTRUSIVE_PTR_HPP
#define SYWU_INTRUSIVE_PTR_HPP

#ifdef SYWU_USE_BOOST
#include <boost/intrusive_ptr.hpp>
#else
#include <sywu/wrap/algorithm.hpp>
#endif

#include <sywu/wrap/atomic.hpp>
#include <sywu/wrap/utility.hpp>

namespace sywu
{

#ifdef SYWU_USE_BOOST

using boost::intrusive_ptr;

#else

/// A copy of the boost intrusive_ptr<> template class, contains only the necessary functions for the library.
template <typename T>
class intrusive_ptr
{
    using ThisType = intrusive_ptr;

public:
    using element_type = T;

    intrusive_ptr() = default;

    intrusive_ptr(T* ptr, bool addRef = true )
        : m_ptr(ptr)
    {
        if (m_ptr && addRef)
        {
            intrusive_ptr_add_ref(m_ptr);
        }
    }

    intrusive_ptr(const intrusive_ptr<T>& rhs)
        : m_ptr(rhs.m_ptr)
    {
        if (m_ptr)
        {
            intrusive_ptr_add_ref(m_ptr);
        }
    }

    intrusive_ptr(intrusive_ptr&& rhs)
    {
        swap(rhs);
    }

    ~intrusive_ptr()
    {
        if (m_ptr)
        {
            intrusive_ptr_release(m_ptr);
        }
    }

    template<class U>
    intrusive_ptr& operator=(const intrusive_ptr<U>& rhs)
    {
        ThisType(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr& operator=(const intrusive_ptr& rhs)
    {
        ThisType(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr& operator=(intrusive_ptr&& rhs)
    {
        ThisType(move(rhs)).swap(*this);
        return *this;
    }

    intrusive_ptr& operator=(T* rhs)
    {
        ThisType(rhs).swap(*this);
        return *this;
    }

    void reset()
    {
        ThisType().swap(*this);
    }

    void reset(T* rhs)
    {
        ThisType(rhs).swap(*this);
    }

    T* get() const
    {
        return m_ptr;
    }

    T& operator*() const
    {
        SYWU_ASSERT(m_ptr);
        return *m_ptr;
    }

    T* operator->() const
    {
        SYWU_ASSERT(m_ptr);
        return m_ptr;
    }

    operator bool() const
    {
        return m_ptr != nullptr;
    }

    void swap(intrusive_ptr& rhs)
    {
        sywu::swap(m_ptr, rhs.m_ptr);
    }

private:
    T* m_ptr = nullptr;
};

#endif

class enable_intrusive_ptr;

template <typename T>
void intrusive_ptr_add_ref(T* ptr)
{
    ++ptr->m_refCount;
}

template <typename T>
void intrusive_ptr_release(T* ptr)
{
    if (--ptr->m_refCount == 0)
    {
        delete ptr;
    }
}

class enable_intrusive_ptr
{
    atomic_int m_refCount = 0;
    template <typename T> friend void intrusive_ptr_add_ref(T*);
    template <typename T> friend void intrusive_ptr_release(T*);

public:
    explicit enable_intrusive_ptr() = default;
    ~enable_intrusive_ptr() = default;

protected:
    int getRefCount() const
    {
        return m_refCount;
    }
};


/// Utility function, creates an intrusive pointer.
template <class T, typename... Arguments>
intrusive_ptr<T> make_intrusive(Arguments&&... arguments)
{
    return move(intrusive_ptr<T>(new T(forward<Arguments>(arguments)...)));
}

}

#endif // SYWU_INTRUSIVE_PTR_HPP
