#ifndef COMP_INTRUSIVE_PTR_HPP
#define COMP_INTRUSIVE_PTR_HPP

#ifdef COMP_USE_BOOST
#include <boost/intrusive_ptr.hpp>
#else
#include <comp/wrap/algorithm.hpp>
#endif

#include <comp/wrap/atomic.hpp>
#include <comp/wrap/exception.hpp>
#include <comp/wrap/type_traits.hpp>
#include <comp/wrap/utility.hpp>

namespace comp
{

#ifdef COMP_USE_BOOST

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

    intrusive_ptr(const intrusive_ptr& rhs)
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
        COMP_ASSERT(m_ptr);
        return *m_ptr;
    }

    T* operator->() const
    {
        COMP_ASSERT(m_ptr);
        return m_ptr;
    }

    operator bool() const
    {
        return m_ptr != nullptr;
    }

    void swap(intrusive_ptr& rhs)
    {
        comp::swap(m_ptr, rhs.m_ptr);
    }

private:
    T* m_ptr = nullptr;
};

#endif

/// Detect intrusive_ptr
template <typename T, typename = void>
struct is_intrusive_ptr : false_type {};

template <typename T>
struct is_intrusive_ptr<T, enable_if_t<is_same_v<decay_t<T>, intrusive_ptr<typename decay_t<T>::element_type>>>> : true_type {};

template <typename T>
constexpr bool is_intrusive_ptr_v = is_intrusive_ptr<T>::value;

/// Template fuction to increase the reference counter of a reference counted \a pointer. The pointer must have an
/// \e m_refCount member that holds the reference count of the object.
/// For convenience, derive your intrusive pointers from #enable_intrusive_ptr.
///
/// \tparam T The pointer type
/// \param pointer The pointer for which to increase the reference counter.
template <typename T>
void intrusive_ptr_add_ref(T* pointer)
{
    ++pointer->m_refCount;
}

/// Template fuction to decrease the reference counter of a reference counted \a pointer, and when that reaches 0, deletes
/// the pointer. The pointer must have an \e m_refCount member that holds the reference count of the object.
/// For convenience, derive your intrusive pointers from #enable_intrusive_ptr.
///
/// \tparam T The pointer type
/// \param pointer The pointer for which to decrease the reference counter.
template <typename T>
void intrusive_ptr_release(T* pointer)
{
    if (--pointer->m_refCount == 0)
    {
        delete pointer;
    }
}

/// Enables the use of intrusive pointers. Derive your class from this to use intrusive pointers as connection trackers.

class enable_intrusive_ptr
{
    atomic_int m_refCount = 0;
    template <typename T> friend void intrusive_ptr_add_ref(T*);
    template <typename T> friend void intrusive_ptr_release(T*);

public:
    /// Constructor.
    explicit enable_intrusive_ptr() = default;
    explicit enable_intrusive_ptr(const enable_intrusive_ptr& other)
        : m_refCount(other.m_refCount.load())
    {
    }
    /// Destructor.
    ~enable_intrusive_ptr()
    {
        if (m_refCount)
        {
            terminate();
        }
    }

    /// Returns an intrusive pointer from this.
    /// \tparam DerivedClass The derived class to create an intrusive ppointer from this object.
    /// \param addReference To increase the reference counter of the intrusive pointrer, pass \e true,
    ///        otherwise pass \e false as argument.
    /// \return The intrusive pointer from this.
    template <class DerivedClass>
    intrusive_ptr<DerivedClass> intrusive_from_this(bool addReference = true) const
    {
        static_assert(is_base_of_v<enable_intrusive_ptr, DerivedClass>, "DerivedClass is not an intrusive pointer base.");
        return move(intrusive_ptr<DerivedClass>(this, addReference));
    }

protected:
    /// Returns the reference counter of the intrusive pointer.
    /// \return The reference counter of the intrusive pointer.
    int getRefCount() const
    {
        return m_refCount;
    }
};


/// Utility function, creates an intrusive pointer.
/// \tparam T The type from which to create the intrusive pointer.
/// \tparam Arguments... The argument types to pass to the type \e T constructor.
/// \param arguments The arguments to pass to the type \e T constructor.
/// \return The intrusive_ptr<T> created.
template <class T, typename... Arguments>
intrusive_ptr<T> make_intrusive(Arguments&&... arguments)
{
    return move(intrusive_ptr<T>(new T(forward<Arguments>(arguments)...)));
}

} // namespace comp

#endif // COMP_INTRUSIVE_PTR_HPP
