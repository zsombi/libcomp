#ifndef SYWU_MUTEX_HPP
#define SYWU_MUTEX_HPP

#include <sywu/wrap/atomic.hpp>
#include <sywu/wrap/utility.hpp>
#include <sywu/config.hpp>

#ifdef SYWU_CONFIG_THREAD_ENABLED

#include <mutex>

#endif

namespace sywu
{

#ifdef SYWU_CONFIG_THREAD_ENABLED

using std::mutex;
using std::lock_guard;

#else

template <class Mutex>
class SYWU_TEMPLATE_API lock_guard
{
public:
    using mutex_type = Mutex;

    explicit lock_guard(mutex_type& m)
        : m_mutex(m)
    {
        m_mutex.lock();
    }
    ~lock_guard()
    {
        m_mutex.unlock();
    }

    SYWU_DISABLE_COPY(lock_guard);
private:
    mutex_type& m_mutex;
};

#endif

/// Unlocks a mutex on creation, and re-locks it on destruction.
template <class Mutex>
class SYWU_TEMPLATE_API relock_guard
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

    SYWU_DISABLE_COPY(relock_guard);
private:
    mutex_type& m_mutex;
};

/// Flag lock. Implements a simple boolean lock guard.
struct SYWU_API FlagGuard : protected atomic_bool
{
    explicit FlagGuard()
    {
        store(false);
    }
    void lock()
    {
        const auto test = try_lock();
        SYWU_ASSERT(test);
    }
    bool try_lock()
    {
        if (!load())
        {
            store(true);
            return true;
        }
        return false;
    }
    void unlock()
    {
        SYWU_ASSERT(load());
        store(false);
    }
    bool isLocked()
    {
        return load();
    }
};

/// Implements a lockable object.
class SYWU_API Lockable
{
#ifdef SYWU_CONFIG_THREAD_ENABLED
    mutable mutex m_mutex;
#else
    FlagGuard m_mutex;
#endif

    SYWU_DISABLE_COPY_OR_MOVE(Lockable)

public:
    explicit Lockable() = default;
    ~Lockable() = default;

    void lock()
    {
        m_mutex.lock();
    }

    void unlock()
    {
        m_mutex.unlock();
    }

    bool try_lock()
    {
        return m_mutex.try_lock();
    }
};

} // namespace sywu

#endif // SYWU_MUTEX_HPP
