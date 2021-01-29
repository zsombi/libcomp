#ifndef COMP_MUTEX_HPP
#define COMP_MUTEX_HPP

#include <comp/wrap/atomic.hpp>
#include <comp/wrap/utility.hpp>
#include <comp/config.hpp>

#ifdef COMP_CONFIG_THREAD_ENABLED

#include <mutex>

#else

#include <comp/wrap/functional.hpp>
#include <comp/wrap/tuple.hpp>

#endif

namespace comp
{

#ifdef COMP_CONFIG_THREAD_ENABLED

using std::mutex;
using std::lock_guard;
using std::scoped_lock;

#else

template <class Mutex>
class COMP_TEMPLATE_API lock_guard
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

    COMP_DISABLE_COPY(lock_guard);
private:
    mutex_type& m_mutex;
};

#endif

/// Unlocks a mutex on creation, and re-locks it on destruction.
template <class Mutex>
class COMP_TEMPLATE_API relock_guard
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

    COMP_DISABLE_COPY(relock_guard);
private:
    mutex_type& m_mutex;
};

/// Flag lock. Implements a simple boolean lock guard.
struct COMP_API FlagGuard : protected atomic_bool
{
    explicit FlagGuard()
    {
        store(false);
    }
    void lock()
    {
        const auto test = try_lock();
        COMP_ASSERT(test);
    }
    bool try_lock()
    {
        const auto state = exchange(true);
        return !state;
    }
    void unlock()
    {
        const auto state = exchange(false);
        COMP_ASSERT(state);
    }
    bool isLocked()
    {
        return load();
    }
};

#ifndef COMP_CONFIG_THREAD_ENABLED
using mutex = FlagGuard;
#endif

} // namespace comp

#endif // COMP_MUTEX_HPP
