#ifndef SYWU_CONNECTION_HPP
#define SYWU_CONNECTION_HPP

#include <sywu/config.hpp>
#include <sywu/wrap/memory.hpp>
#include <sywu/wrap/mutex.hpp>
#include <sywu/wrap/vector.hpp>
#include <sywu/wrap/type_traits.hpp>
#include <sywu/wrap/function_traits.hpp>

namespace sywu
{

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

/// The Slot holds the invocable connected to a signal. The slot is a function, a function object, a method
/// or an other signal.
class SYWU_API Slot : public Lockable<FlagGuard>, public enable_shared_from_this<Slot>
{
    SYWU_DISABLE_COPY_OR_MOVE(Slot);

public:
    /// Destructor.
    virtual ~Slot() = default;

    /// Checks whether a slot is connected.
    /// \return If the slot is connected, returns \e true, otherwise returns \e false.
    bool isConnected() const
    {
        if (!m_isConnected.load())
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
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /// Disconnects a slot.
    void deactivate()
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
            tracker->detach(shared_from_this());
        };
        for_each(m_trackers, detacher);
        m_trackers.clear();
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
    explicit Slot() = default;

    /// The binded trackers.
    TrackersContainer m_trackers;

    atomic_bool m_isConnected = true;
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

    void disconnect()
    {
        auto slot = m_slot.lock();
        if (!slot || !slot->isConnected())
        {
            return;
        }
        slot->deactivate();
    }

    /// Returns the valid state of the connection.
    /// \return If the connection is valid, returns \e true, otherwise returns \e false. A connection is invalid when its
    /// source signal or its trackers are destroyed.
    operator bool() const
    {
        const auto slot = m_slot.lock();
        return slot && slot->isConnected();
    }

    /// Binds trackables to the slot.
    /// \param trackables... The trackables to bind.
    template <class... Trackables>
    Connection& bind(Trackables... trackables);

    /// Returns the slot of the connection.
    /// \return The slot of the connection. If the connection is not valid, returns \e nullptr.
    SlotPtr get() const
    {
        return m_slot.lock();
    }

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
        m_trackedSlots.push_back(Connection(slot));
    }

    /// Detaches the slot from the trackable.
    void detach(SlotPtr slot)
    {
        erase_first(m_trackedSlots, Connection(slot));
    }

protected:
    /// Disconnects the attached slots. Call this method if you want to disconnect from the attached slot
    /// earlier than at the trackable destruction time.
    void disconnectTrackedSlots()
    {
        while (!m_trackedSlots.empty())
        {
            auto connection = m_trackedSlots.back();
            m_trackedSlots.pop_back();

            connection.disconnect();
        }
    }

private:
    vector<Connection> m_trackedSlots;
    atomic_int m_refCount = 0;
};

/********************************************************************************
 *
 */

/// The SignalConcept defines the concept of the signals. Defined as a lockable for convenience, holds the
/// connections of the signal.
template <class LockType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalConcept : public Lockable<LockType>, public Trackable
{
public:
    using SlotType = SlotImpl<ReturnType, Arguments...>;
    using SlotTypePtr = shared_ptr<SlotType>;
    using SignalConceptType = SignalConcept<LockType, ReturnType, Arguments...>;

    /// Destructor.
    ~SignalConcept();

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

    /// Activates the signal with the given arguments.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections invoked.
    size_t operator()(Arguments... arguments);

    /// Adds a \a slot to the signal.
    /// \param slot The slot to add to the signal.
    /// \return The connection token with the signal and the slot.
    Connection addSlot(SlotPtr slot);

    /// Connects a \a method of a \a receiver to this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return Returns the shared pointer to the connection.
    template <class FunctionType>
    enable_if_t<is_member_function_pointer_v<FunctionType>, Connection>
    connect(shared_ptr<typename function_traits<FunctionType>::object> receiver, FunctionType method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param slot The function, functor or lambda to connect.
    /// \return Returns the shared pointer to the connection.
    template <class FunctionType>
    enable_if_t<!is_base_of_v<SignalConceptType, FunctionType>, Connection>
    connect(const FunctionType& function);

    /// Creates a connection between this signal and a \a receiver signal.
    /// \param receiver The receiver signal connected to this signal.
    /// \return Returns the shared pointer to the connection.
    template <class RDerivedClass, typename RReturnType, class... RArguments>
    Connection connect(SignalConcept<RDerivedClass, RReturnType, RArguments...>& receiver);

    /// Disconnects the \a connection passed as argument.
    /// \param connection The connection to disconnect. The connection is invalidated and removed from the signal.
    void disconnect(Connection connection);

protected:
    /// Constructor.
    explicit SignalConcept() = default;

    /// The container of the connections.
    FlagGuard m_emitGuard;

    /// The container with the connected slots.
    using SlotContainer = vector<SlotPtr>;
    SlotContainer m_slots;

private:
    atomic_bool m_isBlocked = false;
};

} // namespace sywu

#endif // SYWU_CONNECTION_HPP
