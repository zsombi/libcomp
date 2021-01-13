#ifndef COMP_CONNECTION_IMPL_HPP
#define COMP_CONNECTION_IMPL_HPP

#include <comp/concept/signal.hpp>
#include <comp/wrap/exception.hpp>
#include <comp/wrap/intrusive_ptr.hpp>

namespace comp
{

namespace
{

template <typename T>
constexpr bool is_trackable_class_v = is_base_of_v<TrackerInterface, decay_t<T>>;

template <typename T>
constexpr bool is_trackable_pointer_v = is_pointer_v<T> && is_trackable_class_v<remove_pointer_t<T>>;

template <typename T>
constexpr bool is_intrusive_trackable_pointer_v = is_intrusive_ptr_v<T> && is_trackable_class_v<typename pointer_traits<T>::element_type>;

template <typename T>
constexpr bool is_valid_trackable_arg = (
            is_trackable_class_v<T> ||
            is_trackable_pointer_v<T> ||
            is_intrusive_trackable_pointer_v<T> ||
            is_weak_ptr_v<T> ||
            is_shared_ptr_v<T>
            );

struct PtrTracker final : TrackerInterface
{
    TrackerInterface* trackable = nullptr;
    explicit PtrTracker(Tracker* trackable)
        : trackable(trackable)
    {
    }
    void track(SlotPtr slot)
    {
        trackable->track(slot);
    }
    void untrack(SlotPtr slot)
    {
        trackable->untrack(slot);
    }
    bool isValid() const
    {
        return true;
    }
};

template <class Type>
struct WeakPtrTracker final : TrackerInterface
{
    weak_ptr<Type> trackable;
    explicit WeakPtrTracker(weak_ptr<Type> trackable)
        : trackable(trackable)
    {
    }
    void track(SlotPtr slot)
    {
        COMP_UNUSED(slot);
        if constexpr (is_trackable_class_v<Type>)
        {
            trackable.lock()->track(slot);
        }
    }
    void untrack(SlotPtr slot)
    {
        COMP_UNUSED(slot);
        if constexpr (is_trackable_class_v<Type>)
        {
            auto locked = trackable.lock();
            if (locked)
            {
                locked->untrack(slot);
            }
        }
    }
    bool isValid() const
    {
        return trackable.lock() != nullptr;
    }
};

template <class Type>
struct IntrusivePtrTracker final : TrackerInterface
{
    intrusive_ptr<Type> trackable;
    explicit IntrusivePtrTracker(intrusive_ptr<Type> trackable)
        : trackable(trackable)
    {
    }
    void track(SlotPtr slot)
    {
        trackable->track(slot);
    }
    void untrack(SlotPtr slot)
    {
        trackable->untrack(slot);
    }
    bool isValid() const
    {
        return trackable;
    }
};

} // noname


template <typename TrackerType>
void SlotInterface::bind(TrackerType tracker)
{
    static_assert (is_valid_trackable_arg<TrackerType>, "Invalid trackable");

    if constexpr (is_trackable_pointer_v<TrackerType>)
    {
        auto _tracker = make_unique<PtrTracker>(tracker);
        _tracker->track(shared_from_this());
        addTracker(move(_tracker));
    }
    else if constexpr (is_weak_ptr_v<TrackerType> || is_shared_ptr_v<TrackerType>)
    {
        using Type = typename pointer_traits<TrackerType>::element_type;
        auto _tracker = make_unique<WeakPtrTracker<Type>>(tracker);
        if constexpr (is_trackable_class_v<Type>)
        {
            _tracker->track(shared_from_this());
        }
        addTracker(move(_tracker));
    }
    else if constexpr (is_intrusive_ptr_v<TrackerType>)
    {
        using Type = typename pointer_traits<TrackerType>::element_type;
        auto _tracker = make_unique<IntrusivePtrTracker<Type>>(tracker);
        _tracker->track(shared_from_this());
        addTracker(move(_tracker));
    }
}


template <typename LockType, typename ReturnType, typename... Arguments>
void SlotConcept<LockType, ReturnType, Arguments...>::addTracker(TrackerPtr&& tracker)
{
    lock_guard lock(*this);
    m_trackers.push_back(forward<TrackerPtr>(tracker));
}

template <typename LockType, typename ReturnType, typename... Arguments>
bool SlotConcept<LockType, ReturnType, Arguments...>::isConnected() const
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

template <typename LockType, typename ReturnType, typename... Arguments>
void SlotConcept<LockType, ReturnType, Arguments...>::disconnect()
{
    lock_guard lock(*this);
    auto isConnected = m_isConnected.exchange(false);
    if (!isConnected)
    {
        // Already disconnected.
        return;
    }

    auto detacher = [this](auto& tracker)
    {
        tracker->untrack(shared_from_this());
    };
    for_each(m_trackers, detacher);
    m_trackers.clear();
}

template <typename LockType, typename ReturnType, typename... Arguments>
ReturnType SlotConcept<LockType, ReturnType, Arguments...>::activate(Arguments&&... args)
{
    if (!isConnected())
    {
        throw bad_slot();
    }

    return activateOverride(forward<Arguments>(args)...);
}

template <class... Trackers>
Connection& Connection::bind(Trackers... trackers)
{
    COMP_ASSERT(*this);
    auto slot = m_slot.lock();
    COMP_ASSERT(slot);

    auto binder = [&slot](auto tracker)
    {
        slot->bind(tracker);
    };
    for_each_arg(binder, trackers...);
    return *this;
}

} // namespace comp

#endif // COMP_CONNECTION_IMPL_HPP
