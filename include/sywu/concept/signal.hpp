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

struct Tracker;
using TrackerPtr = unique_ptr<Tracker>;

class Slot;
using SlotPtr = shared_ptr<Slot>;
using SlotWeakPtr = weak_ptr<Slot>;

/// The Slot holds the invocable connected to a signal. The slot hosts a function, a function object, a method
/// or an other signal.
class SYWU_API Slot : public Lockable, public enable_shared_from_this<Slot>
{
    SYWU_DISABLE_COPY_OR_MOVE(Slot);

public:
    /// Destructor.
    virtual ~Slot() = default;

    /// Checks the validity of a slot.
    /// \return If the slot is valid, returns \e true, otherwise returns \e false.
    bool isValid() const
    {
        auto release = [](auto& tracker) { tracker->release(tracker.get()); };
        auto it = find_if(m_trackers, [](auto& tracker) { return !tracker->retain(tracker.get()); });
        if (it != m_trackers.end())
        {
            // Release the retained ones
            for_each(m_trackers.begin(), it, release);
            return false;
        }

        // Release all, there is no weak_ptr locker that would disturb.
        for_each(m_trackers, release);
        return isValidOverride();
    }

    /// Disconnects a slot.
    void disconnect()
    {
        lock_guard lock(*this);
        auto detacher = [this](auto& tracker)
        {
            tracker->detach(tracker.get(), shared_from_this());
        };
        for_each(m_trackers, detacher);
        m_trackers.clear();
        disconnectOverride();
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

    /// Constructor.
    explicit Slot() = default;

    /// To implement slot specific validation, override this method.
    virtual bool isValidOverride() const = 0;
    /// To implement slot specific disconnect, override this method.
    virtual void disconnectOverride() = 0;

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
    /// To implement slot specific activation, override this method.
    virtual ReturnType activateOverride(Arguments&&...) = 0;
};

/// The Connection holds a slot connected to a signal. It is a token to a sender signal and a receiver
/// slot connected to that signal.
class SYWU_API Connection
{
    friend class SignalConcept;
    template <typename, typename, typename...>
    friend class SignalConceptImpl;

public:
    /// Constructor.
    Connection() = default;

    /// Constructs the connection with the \a sender signal.
    Connection(SignalConcept& sender, SlotPtr slot)
        : m_sender(&sender)
        , m_slot(slot)
    {
    }

    /// Destructor.
    ~Connection() = default;

    /// Disconnect the connection from the signal.
    void disconnect()
    {
        auto slot = m_slot.lock();
        SYWU_ASSERT(slot);
        slot->disconnect();
        m_slot.reset();
        m_sender = nullptr;
    }

    /// Returns the sender signal of the connection.
    /// \return The sender signal. If the connection is invalid, returns \e nullptr.
    SignalConcept* getSender() const
    {
        return m_sender;
    }

    /// Returns the sender signal of the connection.
    /// \tparam SignalType The type fo the signal.
    /// \return The sender signal. If the connection is invalid, or the signal type differs, returns \e nullptr.
    template <class SignalType>
    SignalType* getSender() const
    {
        return static_cast<SignalType*>(m_sender);
    }

    /// Returns the valid state of the connection.
    /// \return If the connection is valid, returns \e true, otherwise returns \e false. A connection is invalid when its
    /// source signal or its trackers are destroyed.
    operator bool() const
    {
        const auto slot = m_slot.lock();
        if (!m_sender || !slot)
        {
            return false;
        }
        return slot->isValid();
    }

    /// Binds trackables to the slot.
    /// \param trackables... The trackables to bind.
    template <class... Trackables>
    Connection& bind(Trackables... trackables);

protected:

    SignalConcept* m_sender = nullptr;
    SlotWeakPtr m_slot;
};

struct ConnectionHelper
{
    /// The current connection. Use this member to access the connection that holds the connected
    /// slot that is activated by the signal.
    static inline Connection currentConnection;
};

/// The SignalConcept defines the concept of the signals. Defined as a lockable for convenience, holds the
/// connections of the signal.
class SYWU_API SignalConcept : public Lockable
{
    SYWU_DISABLE_COPY_OR_MOVE(SignalConcept);
    friend class Connection;

public:
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
        connection.disconnect();
        erase(m_slots, slot);
    }

protected:
    /// Hidden default constructor.
    explicit SignalConcept() = default;

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
    /// Destructor.
    ~Trackable()
    {
        disconnectSlots();
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
        m_slots.push_back(slot);
    }

    /// Detaches the slot from the trackable.
    void detach(SlotPtr slot)
    {
        erase_first(m_slots, slot);
    }

protected:
    /// Constructor.
    explicit Trackable() = default;

    /// Disconnects the attached slot. Call thsi method if you want to disconnect from the attached slot
    /// earlier than at the trackable destruction time.
    void disconnectSlots()
    {
        while (!m_slots.empty())
        {
            auto slot = m_slots.back();
            m_slots.pop_back();
            slot->disconnect();
        }
    }

private:
    vector<SlotPtr> m_slots;
    atomic_int m_refCount = 0;
};

/********************************************************************************
 *
 */

} // namespace sywu

#endif // SYWU_CONNECTION_HPP
