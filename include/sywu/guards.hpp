#ifndef GUARDS_HPP
#define GUARDS_HPP

#include <atomic>
#include <utility>
#include <config.hpp>
#include <sywu/api_defs.hpp>
#include <sywu/extras.hpp>
#include <sywu/type_traits.hpp>

namespace sywu
{

struct BoolLock : protected std::atomic_bool
{
    explicit BoolLock()
    {
        store(false);
    }
    void lock()
    {
        const auto test = try_lock();
        ASSERT(test);
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
        ASSERT(load());
        store(false);
    }
    bool isLocked()
    {
        return load();
    }
};

template <class SharedPtrEnabled>
struct SharedPtrLock
{
    explicit SharedPtrLock(SharedPtrEnabled& lockable)
        : m_lockable(lockable)
    {
    }
    ~SharedPtrLock()
    {
        ASSERT(!m_locked);
    }

    void lock()
    {
        const auto test = try_lock();
        ASSERT(test);
    }
    bool try_lock()
    {
        if (m_locked)
        {
            return false;
        }
        m_locked = m_lockable.shared_from_this();
        return true;
    }
    void unlock()
    {
        ASSERT(m_locked);
        m_locked.reset();
    }
    bool isLocked()
    {
        return m_locked != nullptr;
    }

private:
    SharedPtrEnabled& m_lockable;
    std::shared_ptr<SharedPtrEnabled> m_locked;
};

class SYWU_API Lockable
{
#ifdef CONFIG_THREAD_ENABLED
    mutable std::mutex m_mutex;
#else
    BoolLock m_mutex;
#endif

    DISABLE_COPY_OR_MOVE(Lockable)

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
