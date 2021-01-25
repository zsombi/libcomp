#ifndef COMP_LOCKABLE_HPP
#define COMP_LOCKABLE_HPP

#include <comp/config.hpp>

namespace comp
{

/// Implements a lockable object.
template <typename LockType>
class COMP_TEMPLATE_API Lockable
{
    LockType m_mutex;

    COMP_DISABLE_COPY_OR_MOVE(Lockable)

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

} // comp

#endif // COMP_LOCKABLE_HPP
