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
    }
    void attach(SlotPtr slot) override
    {
        trackable->attach(slot);
    }
    void detach(SlotPtr slot) override
    {
        trackable->detach(slot);
    }
    bool retain() override
    {
        if (!trackable)
        {
            return false;
        }
        return trackable->retain();
    }
    bool release() override
    {
        if (!trackable)
        {
            return true;
        }
        return trackable->release();
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
    }
    ~WeakPtrTracker()
    {
        trackable.reset();
    }
    void attach(SlotPtr) override
    {
    }
    void detach(SlotPtr) override
    {
    }
    bool retain() override
    {
        locked = trackable.lock();
        return locked != nullptr;
    }
    bool release() override
    {
        locked.reset();
        return trackable.use_count() == 0;
    }
};

} // noname


template <class SlotType, bool DisconnectOnRelease>
Slot::Track<SlotType, DisconnectOnRelease>::Track(SlotType& slot)
    : m_slot(slot)
{
    m_lastLocked = find_if(m_slot.m_trackers, [](auto& tracker) { return !tracker->retain(); });
}

template <class SlotType, bool DisconnectOnRelease>
Slot::Track<SlotType, DisconnectOnRelease>::~Track()
{
    auto dirty = false;
    auto release = [&dirty](auto& tracker)
    {
        auto released = tracker->release();
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
        tracker->attach(shared_from_this());
        m_trackers.push_back(move(tracker));
    }
    else if constexpr (is_weak_ptr_v<TrackableType> || is_shared_ptr_v<TrackableType>)
    {
        using Type = typename pointer_traits<TrackableType>::element_type;
        auto tracker = make_unique<WeakPtrTracker<Type>>(trackable);
        tracker->attach(shared_from_this());
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
