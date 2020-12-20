#ifndef SYWU_CONNECTION_IMPL_HPP
#define SYWU_CONNECTION_IMPL_HPP

#include <sywu/connection.hpp>

namespace sywu
{

struct Tracker
{
    // Attaches a slot to a tracker.
    void (*attach)(Tracker*, SlotPtr) = nullptr;
    // Detaches a slot from a tracker.
    void (*detach)(Tracker*, SlotPtr) = nullptr;
    // Retains the tracker. If the retain succeeds, returns true, otherwise false.
    bool (*retain)(Tracker*) = nullptr;
    // Releases the tracker. If the tracker can be deleted, returns true, otherwise false..
    bool (*release)(Tracker*) = nullptr;
};

namespace
{

template <typename T>
constexpr bool is_trackable_class_v = is_base_of_v<Trackable, remove_cvref_t<T>>;

template <typename T>
constexpr bool is_trackable_pointer_v = is_pointer_v<T> && is_trackable_class_v<remove_pointer_t<T>>;

template <typename T>
constexpr bool is_valid_trackable_arg = (
            is_trackable_class_v<T> ||
            is_trackable_pointer_v<T> ||
            is_weak_ptr_v<T> ||
            is_shared_ptr_v<T>
            );

struct TrackablePtrTracker : Tracker
{
    Trackable* trackable = nullptr;
    explicit TrackablePtrTracker(Trackable* trackable)
        : trackable(trackable)
    {
        attach = &TrackablePtrTracker::_attach;
        detach = &TrackablePtrTracker::_detach;
        retain = &TrackablePtrTracker::_retain;
        release = &TrackablePtrTracker::_release;
    }
    ~TrackablePtrTracker()
    {
    }
    static void _attach(Tracker* tracker, SlotPtr slot)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        self->trackable->attach(slot);
    }
    static void _detach(Tracker* tracker, SlotPtr slot)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        self->trackable->detach(slot);
    }
    static bool _retain(Tracker* tracker)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        if (!self || !self->trackable)
        {
            return false;
        }
        return self->trackable->retain();
    }
    static bool _release(Tracker* tracker)
    {
        auto self = static_cast<TrackablePtrTracker*>(tracker);
        if (!self || !self->trackable || self->trackable->release())
        {
            return true;
        }
        return false;
    }
};

template <class Type>
struct WeakPtrTracker : Tracker
{
    weak_ptr<Type> trackable;
    shared_ptr<Type> locked;
    explicit WeakPtrTracker(weak_ptr<Type> trackable)
        : trackable(trackable)
    {
        attach = &WeakPtrTracker::_attach;
        detach = &WeakPtrTracker::_detach;
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
    static void _detach(Tracker*, SlotPtr)
    {
    }
    static bool _retain(Tracker* tracker)
    {
        auto self = static_cast<WeakPtrTracker*>(tracker);
        self->locked = self->trackable.lock();
        return self->locked != nullptr;
    }
    static bool _release(Tracker* tracker)
    {
        auto self = static_cast<WeakPtrTracker*>(tracker);
        self->locked.reset();
        return self->trackable.lock() == nullptr;
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
        tracker = make_unique<TrackablePtrTracker>(trackable);
    }
    else if constexpr (is_trackable_class_v<TrackableType>)
    {
        tracker = make_unique<TrackablePtrTracker>(&trackable);
    }
    else if constexpr (is_weak_ptr_v<TrackableType> || is_shared_ptr_v<TrackableType>)
    {
        using Type = typename pointer_traits<TrackableType>::element_type;
        tracker = make_unique<WeakPtrTracker<Type>>(trackable);
    }
    tracker->attach(tracker.get(), shared_from_this());
    m_trackers.push_back(move(tracker));
}


template <typename ReturnType, typename... Arguments>
ReturnType SlotImpl<ReturnType, Arguments...>::activate(Arguments&&... args)
{
    struct TrackerLock
    {
        SlotImpl& slot;
        TrackersContainer& trackers;
        TrackerLock(SlotImpl& slot, TrackersContainer& trackers)
            : slot(slot)
            , trackers(trackers)
        {
            auto it = find_if(trackers, [](auto& tracker) { return !tracker->retain(tracker.get()); });
            SYWU_ASSERT(it == trackers.end());
        }
        ~TrackerLock()
        {
            bool dirty = false;
            for (auto it = trackers.begin(); it != trackers.end();)
            {
                auto& tracker = *it;
                if (tracker->release(tracker.get()))
                {
                    dirty = true;
                    it = trackers.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (dirty)
            {
                slot.disconnect();
            }
        }
    };

    TrackerLock lock(*this, m_trackers);
    return activateOverride(forward<Arguments>(args)...);
}

template <class... Trackables>
Connection& Connection::bind(Trackables... trackables)
{
    SYWU_ASSERT(*this);
    auto slot = m_slot.lock();
    SYWU_ASSERT(slot);
    lock_guard lock(*slot);

    auto binder = [&slot](auto trackable)
    {
        slot->bind(trackable);
    };
    for_each_arg(binder, trackables...);
    return *this;
}

}

#endif // SYWU_CONNECTION_IMPL_HPP
