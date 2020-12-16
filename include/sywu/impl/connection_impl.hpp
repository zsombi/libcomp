#ifndef SYWU_CONNECTION_IMPL_HPP
#define SYWU_CONNECTION_IMPL_HPP

#include <sywu/connection.hpp>
#include <sywu/type_traits.hpp>

namespace sywu
{

struct Tracker
{
    void (*attach)(Tracker*, SlotPtr) = nullptr;
    bool (*isValid)(const Tracker*) = nullptr;
    bool (*retain)(Tracker*) = nullptr;
    void (*release)(Tracker*) = nullptr;
};

namespace
{

template <typename T>
constexpr bool is_trackable_class_v = std::is_base_of_v<Trackable, traits::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_trackable_pointer_v = std::is_pointer_v<T> && is_trackable_class_v<std::remove_pointer_t<T>>;

template <typename T>
constexpr bool is_valid_trackable_arg = (
            is_trackable_class_v<T> ||
            is_trackable_pointer_v<T> ||
            traits::is_weak_ptr_v<T> ||
            traits::is_shared_ptr_v<T>
            );

struct TrackablePtrTracker : Tracker
{
    Trackable* trackable = nullptr;
    explicit TrackablePtrTracker(Trackable* trackable)
        : trackable(trackable)
    {
        attach = &TrackablePtrTracker::_attach;
        isValid = &TrackablePtrTracker::_isValid;
        retain = &TrackablePtrTracker::_retain;
        release = &TrackablePtrTracker::_release;
    }
    ~TrackablePtrTracker()
    {
        trackable->detach();
        trackable = nullptr;
    }
    static void _attach(Tracker* tracker, SlotPtr slot)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        self->trackable->attach(slot);
    }
    static bool _isValid(const Tracker* tracker)
    {
        auto self = static_cast<const TrackablePtrTracker*>(tracker);
        if (!self)
        {
            return false;
        }
        if (!self->trackable)
        {
            return false;
        }
        return self->trackable->isAttached();
    }
    static bool _retain(Tracker* tracker)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        if (!self || !self->isValid(tracker))
        {
            return false;
        }
        return self->trackable->retain();
    }
    static void _release(Tracker* tracker)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        if (!self || !self->isValid(tracker))
        {
            return;
        }
        self->trackable->release();
    }
};

template <class Type>
struct WeakPtrTracker : Tracker
{
    std::weak_ptr<Type> trackable;
    std::shared_ptr<Type> locked;
    explicit WeakPtrTracker(std::weak_ptr<Type> trackable)
        : trackable(trackable)
    {
        attach = &WeakPtrTracker::_attach;
        isValid = &WeakPtrTracker::_isValid;
        retain = &WeakPtrTracker::_retain;
        release = &WeakPtrTracker::_release;
    }
    ~WeakPtrTracker()
    {
        trackable.reset();
    }
    static void _attach(Tracker*, SlotPtr)
    {
    }
    static bool _isValid(const Tracker* tracker)
    {
        auto self = static_cast<const WeakPtrTracker*>(tracker);
        return !self->trackable.expired();
    }
    static bool _retain(Tracker* tracker)
    {
        auto self = static_cast<WeakPtrTracker*>(tracker);
        self->locked = self->trackable.lock();
        return self->locked != nullptr;
    }
    static void _release(Tracker* tracker)
    {
        auto self = static_cast<WeakPtrTracker*>(tracker);
        self->locked.reset();
    }
};

} // noname


template <typename TrackableType>
void Slot::bind(TrackableType trackable)
{
    static_assert (is_valid_trackable_arg<TrackableType>, "Invalid trackable");
    SYWU_UNUSED(trackable);
    TrackerPtr tracker;
    if constexpr (is_trackable_pointer_v<TrackableType>)
    {
        tracker = std::make_unique<TrackablePtrTracker>(trackable);
    }
    else if constexpr (is_trackable_class_v<TrackableType>)
    {
        tracker = std::make_unique<TrackablePtrTracker>(&trackable);
    }
    else if constexpr (traits::is_weak_ptr_v<TrackableType> || traits::is_shared_ptr_v<TrackableType>)
    {
        using Type = typename std::pointer_traits<TrackableType>::element_type;
        tracker = std::make_unique<WeakPtrTracker<Type>>(trackable);
    }
    tracker->attach(tracker.get(), shared_from_this());
    m_trackers.push_back(std::move(tracker));
}


template <typename ReturnType, typename... Arguments>
ReturnType SlotImpl<ReturnType, Arguments...>::activate(Arguments&&... args)
{
    struct TrackerLock
    {
        TrackersContainer& trackers;
        TrackerLock(TrackersContainer& trackers)
            :trackers(trackers)
        {
            auto it = utils::find_if(trackers, [](auto& tracker) { return !tracker->retain(tracker.get()); });
            SYWU_ASSERT(it == trackers.end());
        }
        ~TrackerLock()
        {
            utils::for_each(trackers, [](auto& tracker) { tracker->release(tracker.get()); });
        }
    };

    TrackerLock lock(m_trackers);
    return activateOverride(std::forward<Arguments>(args)...);
}

template <class... Trackables>
Connection& Connection::bind(Trackables... trackables)
{
    SYWU_ASSERT(isValid());
    auto slot = m_slot.lock();
    SYWU_ASSERT(slot);
    lock_guard lock(*slot);

    auto binder = [&slot](auto trackable)
    {
        slot->bind(trackable);
    };
    utils::for_each_arg(binder, trackables...);
    return *this;
}

}

#endif // SYWU_CONNECTION_IMPL_HPP
