#ifndef COMP_SIGNAL_IMPL_HPP
#define COMP_SIGNAL_IMPL_HPP

#include <comp/concept/core/signal.hpp>

namespace comp { namespace core {

template <typename LockType>
bool Slot<LockType>::isConnected() const
{
    if (!m_isConnected.load())
    {
        return false;
    }

    try
    {
        auto isTrackerValid = [](auto& tracker)
        {
            return !tracker->isValid();
        };
        auto it = find_if(m_trackers, isTrackerValid);
        return (it == m_trackers.cend());
    }
    catch (...)
    {
        return false;
    }
}

template <typename LockType>
void Slot<LockType>::disconnect()
{
    lock_guard lock(*this);
    if (m_signal)
    {
        auto signal = m_signal;
        m_signal = nullptr;
        relock_guard relock(*this);
        signal->disconnect(Signal::Connection(this->shared_from_this()));
    }

    auto isConnected = m_isConnected.exchange(false);
    if (!isConnected)
    {
        // Already disconnected.
        return;
    }

    if (disconnectOverride)
    {
        disconnectOverride();
    }
    m_trackers.clear();
}

template <typename LockType>
void Slot<LockType>::addTracker(TrackerPtr tracker)
{
    lock_guard lock(*this);
    m_trackers.push_back(tracker);
}

}} // comp::core

#endif // COMP_SIGNAL_IMPL_HPP
