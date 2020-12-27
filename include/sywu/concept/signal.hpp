#ifndef SYWU_CONNECTION_HPP
#define SYWU_CONNECTION_HPP

#include <sywu/wrap/memory.hpp>
#include <sywu/config.hpp>
#include <sywu/wrap/vector.hpp>
#include <sywu/wrap/mutex.hpp>
#include <sywu/wrap/type_traits.hpp>

namespace sywu
{

class SignalConcept;
class Connection;
class Slot;
using SlotPtr = shared_ptr<Slot>;
using SlotWeakPtr = weak_ptr<Slot>;

struct SYWU_API Tracker
{
    virtual ~Tracker() = default;
    // Attaches a slot to a tracker.
    virtual void attach(SlotPtr) = 0;
    // Detaches a slot from a tracker.
    virtual void detach(SlotPtr) = 0;
    // Retains the tracker. If the retain succeeds, returns true, otherwise false.
    virtual bool retain() = 0;
    // Releases the tracker. If the tracker can be deleted, returns true, otherwise false..
    virtual bool release() = 0;
};
using TrackerPtr = unique_ptr<Tracker>;

/// The Slot holds the invocable connected to a signal. The slot hosts a function, a function object, a method
/// or an other signal.
class SYWU_API Slot : public Lockable, public enable_shared_from_this<Slot>
{
    SYWU_DISABLE_COPY_OR_MOVE(Slot);

public:
    /// Destructor.
    virtual ~Slot() = default;

    /// Returns the sender signal to which the slot is connected.
    /// \returns The pointer to the sender signal, or \e nullptr if the slot is invalid.
    SignalConcept* getSignal() const
    {
        return m_sender;
    }

    /// Checks whether a slot is active.
    /// \return If the slot is active, returns \e true, otherwise returns \e false.
    bool isActive() const
    {
        if (!m_sender)
        {
            return false;
        }

        try
        {
            Track<const Slot, false> track(*this);
            if (!track.retainedInFull())
            {
                return false;
            }
            return isActiveOverride();
        }
        catch (...)
        {
            return false;
        }
    }

    /// Deactivates a slot.
    void deactivate()
    {
        lock_guard lock(*this);
        if (!m_sender)
        {
            // Already inactive.
            return;
        }

        auto detacher = [this](auto& tracker)
        {
            tracker->detach(shared_from_this());
        };
        for_each(m_trackers, detacher);
        m_trackers.clear();

        deactivateOverride();

        m_sender = nullptr;
    }

    /// Binds a trackable object to the slot. The trackable object is either a shared pointer, a weak pointer,
    /// or a Trackable derived object.
    /// \tparam TrackableType The type of the trackable, a shared_ptr, weak_ptr, or a pointer to the Trackable
    ///         derived object.
    template <class TrackableType>
    void bind(TrackableType trackable);

protected:
    /// The container with the binded trackers.
    using TrackersContainer = vector<TrackerPtr>;

    template <class SlotType, bool DisconnectOnRelease = false>
    struct Track
    {
        SlotType& m_slot;
        TrackersContainer::const_iterator m_lastLocked;
        Track(SlotType& slot);
        ~Track();
        bool retainedInFull() const;
    };

    /// Constructor.
    explicit Slot(SignalConcept& sender)
        : m_sender(&sender)
    {
    }

    /// To implement slot specific validation, override this method.
    virtual bool isActiveOverride() const
    {
        return true;
    }
    /// To implement slot specific disconnect, override this method.
    virtual void deactivateOverride()
    {
    }

    /// The sender signal to which the slot is connected.
    SignalConcept* m_sender = nullptr;

    /// The binded trackers.
    TrackersContainer m_trackers;
};

///The SlotImpl declares the activation of a slot.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SlotImpl : public Slot
{
public:
    /// Activates the slot with the arguments passed, and returns the slot's return value.
    ReturnType activate(Arguments&&...);

protected:
    /// Constructor.
    explicit SlotImpl(SignalConcept& sender);

    /// To implement slot specific activation, override this method.
    virtual ReturnType activateOverride(Arguments&&...) = 0;
};

/// The Connection holds a slot connected to a signal. It is a token to a sender signal and a receiver
/// slot connected to that signal.
class SYWU_API Connection
{
    friend class SignalConcept;

public:
    /// Constructor.
    Connection() = default;

    /// Constructs the connection with a \a slot.
    Connection(SlotPtr slot)
        : m_slot(slot)
    {
    }

    /// Destructor.
    ~Connection() = default;

    /// Returns the sender signal of the connection.
    /// \return The sender signal. If the connection is invalid, returns \e nullptr.
    SignalConcept* getSender() const
    {
        const auto slot = m_slot.lock();
        return slot ? slot->getSignal() : nullptr;
    }

    /// Returns the sender signal of the connection.
    /// \tparam SignalType The type fo the signal.
    /// \return The sender signal. If the connection is invalid, or the signal type differs, returns \e nullptr.
    template <class SignalType>
    SignalType* getSender() const
    {
        const auto slot = m_slot.lock();
        return slot ? static_cast<SignalType*>(slot->getSignal()) : nullptr;
    }

    /// Returns the valid state of the connection.
    /// \return If the connection is valid, returns \e true, otherwise returns \e false. A connection is invalid when its
    /// source signal or its trackers are destroyed.
    operator bool() const
    {
        const auto slot = m_slot.lock();
        return slot && slot->isActive();
    }

    /// Binds trackables to the slot.
    /// \param trackables... The trackables to bind.
    template <class... Trackables>
    Connection& bind(Trackables... trackables);

private:
    SlotWeakPtr m_slot;
};

/// This class provides the token of the active connection. You can use the members of this class to get information
/// about the emitter signal, and to manipulate the signal as well as to disconnect the slot.
struct ActiveConnection
{
    /// The current connection. Use this member to access the connection that holds the connected
    /// slot that is activated by the signal.
    static inline Connection connection;
};

/// The SignalConcept defines the concept of the signals. Defined as a lockable for convenience, holds the
/// connections of the signal.
class SYWU_API SignalConcept : public Lockable
{
    SYWU_DISABLE_COPY_OR_MOVE(SignalConcept);
    friend class Connection;

public:
    /// Destructor.
    ~SignalConcept()
    {
        lock_guard lock(*this);

        while (!m_slots.empty())
        {
            auto slot = m_slots.back();
            m_slots.pop_back();
            slot->deactivate();
        }
    }

    /// Returns the blocked state of a signal.
    /// \return The blocked state of a signal. When a signal is blocked, the signal emission does nothing.
    bool isBlocked() const
    {
        return m_isBlocked.load();
    }

    /// Sets the \a blocked state of a signal.
    /// \param blocked The new blocked state of a signal.
    void setBlocked(bool blocked)
    {
        m_isBlocked = blocked;
    }

    /// Disconnects the \a connection passed as argument.
    /// \param connection The connection to disconnect. The connection is invalidated and removed from the signal.
    void disconnect(Connection connection)
    {
        lock_guard lock(*this);
        auto slot = connection.m_slot.lock();
        if (!slot)
        {
            return;
        }

        slot->deactivate();
        erase(m_slots, slot);
    }

protected:
    /// Hidden default constructor.
    explicit SignalConcept() = default;

    /// The container with the connected slots.
    using SlotContainer = vector<SlotPtr>;
    SlotContainer m_slots;

private:
    atomic_bool m_isBlocked = false;
};

/// To track the lifetime of a connection based on an arbitrary object that is not a smart pointer, use this class.
/// You can use this class as a base class
class SYWU_API Trackable
{
public:
    friend void intrusive_ptr_add_ref(Trackable* object)
    {
        object->retain();
    }
    friend void intrusive_ptr_release(Trackable* object)
    {
        if (object->release())
        {
            delete object;
        }

    }

    /// Constructor.
    explicit Trackable() = default;

    /// Destructor.
    ~Trackable()
    {
        disconnectTrackedSlots();
        m_refCount = 0;
    }

    /// Retains the trackable.
    bool retain()
    {
        ++m_refCount;
        return m_refCount.load() > 0;
    }

    /// Releases the trackable.
    /// \returns If the object should be released, returns true.
    bool release()
    {
        --m_refCount;
        return (m_refCount <= 0);
    }

    /// Attaches a \a slot to the trackable.
    void attach(SlotPtr slot)
    {
        m_trackedSlots.push_back(slot);
    }

    /// Detaches the slot from the trackable.
    void detach(SlotPtr slot)
    {
        erase_first(m_trackedSlots, slot);
    }

protected:
    /// Disconnects the attached slots. Call this method if you want to disconnect from the attached slot
    /// earlier than at the trackable destruction time.
    void disconnectTrackedSlots()
    {
        while (!m_trackedSlots.empty())
        {
            auto slot = m_trackedSlots.back();
            m_trackedSlots.pop_back();
            auto signal = slot->getSignal();
            if (signal)
            {
                signal->disconnect({slot});
            }
        }
    }

private:
    vector<SlotPtr> m_trackedSlots;
    atomic_int m_refCount = 0;
};

/********************************************************************************
 *
 */

} // namespace sywu

#endif // SYWU_CONNECTION_HPP
