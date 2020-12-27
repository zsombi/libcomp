#ifndef SYWU_CONNECTION_IMPL_HPP
#define SYWU_CONNECTION_IMPL_HPP

#include <sywu/concept/signal.hpp>
#include <sywu/wrap/exception.hpp>
#include <sywu/wrap/intrusive_ptr.hpp>

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
constexpr bool is_trackable_class_v = is_base_of_v<Trackable, decay_t<T>>;

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
        if (!self || !self->trackable)
        {
            return true;
        }
        return self->trackable->release();
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
        return self->trackable.use_count() == 0;
    }
};

} // noname


template <class SlotType, bool DisconnectOnRelease>
Slot::Track<SlotType, DisconnectOnRelease>::Track(SlotType& slot)
    : m_slot(slot)
{
    m_lastLocked = find_if(m_slot.m_trackers, [](auto& tracker) { return !tracker->retain(tracker.get()); });
}

template <class SlotType, bool DisconnectOnRelease>
Slot::Track<SlotType, DisconnectOnRelease>::~Track()
{
    auto dirty = false;
    auto release = [&dirty](auto& tracker)
    {
        auto released = tracker->release(tracker.get());
        dirty |= released;
        return released;
    };
    if constexpr (DisconnectOnRelease)
    {
        erase_if(m_slot.m_trackers, release);
        if (dirty)
        {
            m_slot.deactivate();
        }
    }
    else
    {
        for_each(m_slot.m_trackers.begin(), m_lastLocked, release);
    }
}

template <class SlotType, bool DisconnectOnRelease>
bool Slot::Track<SlotType, DisconnectOnRelease>::retainedInFull() const
{
    return m_lastLocked == m_slot.m_trackers.end();
}


template <typename TrackableType>
void Slot::bind(TrackableType trackable)
{
    static_assert (is_valid_trackable_arg<TrackableType>, "Invalid trackable");

    if constexpr (is_trackable_pointer_v<TrackableType>)
    {
        auto tracker = make_unique<TrackablePtrTracker>(trackable);
        tracker->attach(tracker.get(), shared_from_this());
        m_trackers.push_back(move(tracker));
    }
    else if constexpr (is_weak_ptr_v<TrackableType> || is_shared_ptr_v<TrackableType>)
    {
        using Type = typename pointer_traits<TrackableType>::element_type;
        auto tracker = make_unique<WeakPtrTracker<Type>>(trackable);
        tracker->attach(tracker.get(), shared_from_this());
        m_trackers.insert(m_trackers.begin(), move(tracker));
    }
}


template <typename ReturnType, typename... Arguments>
SlotImpl<ReturnType, Arguments...>::SlotImpl(SignalConcept& sender)
    : Slot(sender)
{
}

template <typename ReturnType, typename... Arguments>
ReturnType SlotImpl<ReturnType, Arguments...>::activate(Arguments&&... args)
{
    if (!m_sender)
    {
        throw bad_slot();
    }

    Track<Slot, true> retain(*this);
    if (!retain.retainedInFull())
    {
        throw bad_slot();
    }

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
