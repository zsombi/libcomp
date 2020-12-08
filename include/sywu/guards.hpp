#ifndef GUARDS_HPP
#define GUARDS_HPP

#include <atomic>
#include <utility>
#include <config.hpp>
#include <sywu/api_defs.hpp>
#include <sywu/type_traits.hpp>

#ifdef DEBUG
#include <cassert>
#endif

namespace sywu
{

template <typename T>
void SYWU_TEMPLATE_API default_retain(traits::remove_cvref_t<T>& v)
{
    ++v;
}

template <typename T>
void SYWU_TEMPLATE_API default_release(traits::remove_cvref_t<T>& v)
{
    --v;
}

template <typename T>
void SYWU_TEMPLATE_API default_atomic_retain(std::atomic<traits::remove_cvref_t<T>>& v)
{
    ++v;
}

template <typename T>
void SYWU_TEMPLATE_API default_atomic_release(std::atomic<traits::remove_cvref_t<T>>& v)
{
    --v;
}

/// Template for reference counted elements, with a retain and release functions.
template <typename T,
          void (*RetainFunction)(traits::remove_cvref_t<T>&) = default_retain<T>,
          void (*ReleaseFunction)(traits::remove_cvref_t<T>&) = default_release<T>>
class SYWU_TEMPLATE_API RefCounted
{
    using Type = traits::remove_cvref_t<T>;

public:
    /// Construct the reference counted from a value.
    RefCounted(const Type& t = Type())
        : m_value(t)
    {
    }
    /// Move constructor.
    RefCounted(RefCounted&& other)
        : m_value(other.m_value)
    {
        other.value = Type();
    }
    /// Copy constructor.
    RefCounted(const RefCounted& other)
        : m_value(other.m_value)
    {
        RetainFunction(m_value);
    }
    /// Destructor.
    ~RefCounted()
    {
        ReleaseFunction(m_value);
    }
    /// Swaps two reference counted values.
    void swap(RefCounted& other) noexcept
    {
        std::swap(m_value, other.m_value);
    }
    /// Copy assignment operator.
    RefCounted& operator=(const RefCounted& other)
    {
        RefCounted copy(other);
        swap(copy);
        return *this;
    }
    /// Move assignment operator.
    RefCounted& operator=(RefCounted&& other)
    {
        RefCounted moved(std::move(other));
        swap(moved);
        return *this;
    }
    /// Retain count.
    T getRetainCount() const
    {
        return m_value;
    }
    /// Self retainer function.
    void retain()
    {
        RetainFunction(m_value);
    }
    /// Self release function.
    void release()
    {
        ReleaseFunction(m_value);
    }

private:
    /// The reference counted value.
    T m_value;
};

/// Template for reference counted atomic elements, with a retain and release functions.
template <typename T,
          void (*RetainFunction)(std::atomic<traits::remove_cvref_t<T>>&) = default_atomic_retain<T>,
          void (*ReleaseFunction)(std::atomic<traits::remove_cvref_t<T>>&) = default_atomic_release<T>>
class SYWU_TEMPLATE_API AtomicRefCounted
{
    using RefType = traits::remove_cvref_t<T>;

public:
    /// Construct the reference counted from a value.
    AtomicRefCounted(const RefType& t = RefType())
        : m_value(t)
    {
    }
    /// Move constructor.
    AtomicRefCounted(AtomicRefCounted&& other)
        : m_value(other.m_value)
    {
        other.value = RefType();
    }
    /// Copy constructor.
    AtomicRefCounted(const AtomicRefCounted& other)
        : m_value(other.m_value)
    {
        RetainFunction(m_value);
    }
    /// Destructor.
    ~AtomicRefCounted()
    {
        ReleaseFunction(m_value);
    }
    /// Swaps two reference counted values.
    void swap(AtomicRefCounted& other) noexcept
    {
        std::swap(m_value, other.m_value);
    }
    /// Copy assignment operator.
    AtomicRefCounted& operator=(const AtomicRefCounted& other)
    {
        RefCounted copy(other);
        swap(copy);
        return *this;
    }
    /// Move assignment operator.
    AtomicRefCounted& operator=(AtomicRefCounted&& other)
    {
        AtomicRefCounted moved(std::move(other));
        swap(moved);
        return *this;
    }
    /// Cast operator.
    operator T() const
    {
        return m_value;
    }
    /// Self retainer function.
    void retain()
    {
        RetainFunction(m_value);
    }
    /// Self release function.
    void release()
    {
        ReleaseFunction(m_value);
    }

protected:
    /// The reference counted value.
    std::atomic<RefType> m_value;
};

/// Template for reference counted types.
/// \tparam RefCountedType The reference counted type, must derive from RefCountedType template
/// or at least have retain() and release() methods.
template <class RefCountedType>
class SYWU_TEMPLATE_API RefCounter
{
public:
    /// Constructs a reference counter from \a self, and retains \a self.
    RefCounter(RefCountedType& self)
        : m_refCounted(self)
    {
        m_refCounted.retain();
    }
    /// Destructs the reference counter releasing the reference counted element.
    ~RefCounter()
    {
        m_refCounted.release();
    }

protected:
    /// The reference counted element.
    RefCountedType& m_refCounted;
};


#ifndef DEBUG
class SYWU_API Lockable
#else
class SYWU_API Lockable : public RefCounted<int32_t>
#endif
{
#ifdef CONFIG_THREAD_ENABLED
    mutable std::mutex m_mutex;
#endif

    DISABLE_COPY_OR_MOVE(Lockable)

public:
    explicit Lockable() = default;
    virtual ~Lockable() = default;

    void lock()
    {
#ifdef CONFIG_THREAD_ENABLED
        m_mutex.lock();
#elif DEBUG
        if (!try_lock())
        {
            abort();
        }
#endif
    }

    void unlock()
    {
#ifdef CONFIG_THREAD_ENABLED
        m_mutext.unlock();
#elif DEBUG
        if (getRetainCount() == 0)
        {
            abort();
        }
        release();
#endif
    }

    bool try_lock()
    {
#ifdef CONFIG_THREAD_ENABLED
        return m_mutext.try_lock();
#elif DEBUG
        if (getRetainCount() > 0)
        {
            return false;
        }
        retain();
        return true;
#endif
    }
};

/// Shares a lockable object with elements of that object that must lock the object itself.
template <typename RefLockable>
class SYWU_TEMPLATE_API SharedLock : public RefCounter<RefLockable&>
{
    using BaseClass = RefCounter<RefLockable&>;
    DISABLE_COPY_OR_MOVE(SharedLock)

public:
    /// Construct the shared lock with the given \a sharedLock. Does not retain the lock, but
    /// increases the shared count of the object lock.
    explicit SharedLock(RefLockable& sharedLock)
      : BaseClass(sharedLock)
    {
    }
    /// Destructs the shared lock and releases the object lock shared count.
    virtual ~SharedLock() = default;

    /// Locks the shared object.
    void lock()
    {
        this->m_refCounted.lock();
    }
    /// Unlocks the shared object.
    void unlock()
    {
        this->m_refCounted.unlock();
    }
    /// Tries to lock the shared object.
    /// \return If the object locking succeeds, returns \e true, otherwise \e false.
    bool try_lock()
    {
        return this->m_refCounted.try_lock();
    }

    RefLockable* get() const
    {
        return &this->m_refCounted;
    }
};


/// Lock guard.
#ifdef CONFIG_THREAD_ENABLED
using std::lock_gaurd;
#else
template <class Mutex>
class lock_guard
{
public:
    typedef Mutex mutex_type;

    explicit lock_guard(mutex_type& m)
        : m_mutex(m)
    {
        m_mutex.lock();
    }
    ~lock_guard()
    {
        m_mutex.unlock();
    }

    DISABLE_COPY(lock_guard);
private:
    mutex_type& m_mutex;
};
#endif

/// Unlocks a mutex on creation, and re-locks it on destruction.
template <class Mutex>
class relock_guard
{
public:
    typedef Mutex mutex_type;

    explicit relock_guard(mutex_type& m)
        : m_mutex(m)
    {
        m_mutex.unlock();
    }
    ~relock_guard()
    {
        m_mutex.lock();
    }

    DISABLE_COPY(relock_guard);
private:
    mutex_type& m_mutex;
};


} // namespace sywu

#endif // GUARDS_HPP
