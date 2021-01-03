#ifndef SYWU_CONNECTION_IMPL_HPP
#define SYWU_CONNECTION_IMPL_HPP

#include <sywu/concept/signal.hpp>
#include <sywu/wrap/exception.hpp>
#include <sywu/wrap/intrusive_ptr.hpp>

namespace sywu
{

namespace
{

template <typename T>
constexpr bool is_trackable_class_v = is_base_of_v<Tracker, decay_t<T>>;

template <typename T>
constexpr bool is_trackable_pointer_v = is_pointer_v<T> && is_trackable_class_v<remove_pointer_t<T>>;

template <typename T>
constexpr bool is_valid_trackable_arg = (
            is_trackable_class_v<T> ||
            is_trackable_pointer_v<T> ||
            is_weak_ptr_v<T> ||
            is_shared_ptr_v<T>
            );

struct PtrTracker final : AbstractTracker
{
    Tracker* trackable = nullptr;
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
struct WeakPtrTracker final : AbstractTracker
{
    weak_ptr<Type> trackable;
    explicit WeakPtrTracker(weak_ptr<Type> trackable)
        : trackable(trackable)
    {
    }
    void track(SlotPtr)
    {
    }
    void untrack(SlotPtr)
    {
    }
    bool isValid() const
    {
        return trackable.lock() != nullptr;
    }
};

} // noname


template <typename TrackerType>
void Slot::bind(TrackerType tracker)
{
    static_assert (is_valid_trackable_arg<TrackerType>, "Invalid trackable");

    if constexpr (is_trackable_pointer_v<TrackerType>)
    {
        auto _tracker = make_unique<PtrTracker>(tracker);
        _tracker->track(shared_from_this());
        m_trackers.push_back(move(_tracker));
    }
    else if constexpr (is_weak_ptr_v<TrackerType> || is_shared_ptr_v<TrackerType>)
    {
        using Type = typename pointer_traits<TrackerType>::element_type;
        auto _tracker = make_unique<WeakPtrTracker<Type>>(tracker);
        m_trackers.insert(m_trackers.begin(), move(_tracker));
    }
}


template <typename ReturnType, typename... Arguments>
ReturnType SlotImpl<ReturnType, Arguments...>::activate(Arguments&&... args)
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
    SYWU_ASSERT(*this);
    auto slot = m_slot.lock();
    SYWU_ASSERT(slot);
    lock_guard lock(*slot);

    auto binder = [&slot](auto tracker)
    {
        slot->bind(tracker);
    };
    for_each_arg(binder, trackers...);
    return *this;
}

} // namespace sywu

#endif // SYWU_CONNECTION_IMPL_HPP
