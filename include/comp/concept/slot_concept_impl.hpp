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
constexpr bool is_trackable_class_v = is_base_of_v<ConnectionTracker, decay_t<T>>;

template <typename T>
constexpr bool is_trackable_pointer_v = is_pointer_v<T> && is_trackable_class_v<remove_pointer_t<T>>;

template <typename T>
constexpr bool is_intrusive_trackable_pointer_v = is_intrusive_ptr_v<T> && is_trackable_class_v<typename pointer_traits<T>::element_type>;

template <typename T>
constexpr bool is_valid_trackable_arg = (
            is_trackable_pointer_v<T> ||
            is_intrusive_trackable_pointer_v<T> ||
            is_weak_ptr_v<T> ||
            is_shared_ptr_v<T>
            );

template <typename TrackedType>
struct SlotTracker final : public core::Slot<mutex>::TrackerInterface
{
    using Base = typename core::Slot<mutex>::TrackerInterface;
    using ManagedType = typename pointer_traits<TrackedType>::element_type;
    using PointerType = conditional_t<is_shared_ptr_v<TrackedType>, weak_ptr<ManagedType>, TrackedType>;
    static constexpr bool isTracker = is_trackable_class_v<ManagedType>;

    Connection connection;
    PointerType tracked;

    static auto create(Connection connection, TrackedType tracked)
    {
        auto tracker = make_shared<Base, SlotTracker>(connection, tracked);
        if constexpr (isTracker)
        {
            tracked->track(connection);
        }
        return tracker;
    }

    explicit SlotTracker(Connection connection, TrackedType tracked)
        : connection(connection)
        , tracked(tracked)
    {
    }
    ~SlotTracker()
    {
        untrack();
    }

    void untrack()
    {
        if constexpr (isTracker)
        {
            if constexpr (is_weak_ptr_v<PointerType>)
            {
                auto lock = tracked.lock();
                if (lock)
                {
                    lock->untrack(connection);
                }
            }
            else
            {
                tracked->untrack(connection);
            }
        }
    }

    bool isValid() const
    {
        if constexpr (is_pointer_v<TrackedType>)
        {
            return true;
        }
        else if constexpr (is_weak_ptr_v<PointerType>)
        {
            return tracked.lock() != nullptr;
        }
        else
        {
            return tracked;
        }
    }
};

} // noname



template <typename ReturnType, typename... Arguments>
ReturnType SlotConcept<ReturnType, Arguments...>::activate(Arguments&&... args)
{
    COMP_ASSERT(activateOverride);
    if (!this->isConnected())
    {
        throw bad_slot();
    }

    return activateOverride(forward<Arguments>(args)...);
}

template <class... Trackers>
Connection& Connection::bindTrackers(Trackers... trackers)
{
    COMP_ASSERT(*this);
    auto slot = m_slot.lock();
    COMP_ASSERT(slot);

    auto binder = [this, &slot](auto tracker)
    {
        this->bindTracker(slot, tracker);
    };
    for_each_arg(binder, trackers...);
    return *this;
}

template <class TrackerType>
void Connection::bindTracker(SlotPtr slot, TrackerType tracker)
{
    static_assert (is_valid_trackable_arg<TrackerType>, "Invalid trackable");

    slot->addTracker(SlotTracker<TrackerType>::create(*this, tracker));
}

} // namespace comp

#endif // COMP_CONNECTION_IMPL_HPP
