#ifndef COMP_CONNECTION_HPP
#define COMP_CONNECTION_HPP

#include <comp/config.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/vector.hpp>
#include <comp/wrap/type_traits.hpp>
#include <comp/wrap/function_traits.hpp>

namespace comp
{

class Connection;
class SlotInterface;
using SlotPtr = shared_ptr<SlotInterface>;
using SlotWeakPtr = weak_ptr<SlotInterface>;

/// ConnectionTracker interface.
struct COMP_API TrackerInterface
{
    /// Destructor.
    virtual ~TrackerInterface() = default;
    /// Attaches a slot to a tracker.
    virtual void track(Connection) = 0;
    /// Detaches a slot from a tracker.
    virtual void untrack(Connection) = 0;
    /// Returns the valid state of a tracker. A tracker is valid when it tracks a valid object.
    /// \return If the tracker is valid, returns \e true, otherwise \e false.
    virtual bool isValid() const = 0;
};
using TrackerPtr = unique_ptr<TrackerInterface>;

/// Interface for slots.
class COMP_API SlotInterface : public enable_shared_from_this<SlotInterface>
{
public:
    virtual ~SlotInterface() = default;

    /// Checks whether a slot is connected.
    /// \return If the slot is connected, returns \e true, otherwise returns \e false.
    virtual bool isConnected() const = 0;

    /// Disconnects a slot.
    virtual void disconnect() = 0;

    /// Binds a tracker object to the slot. The tracker object is either a shared pointer, a weak pointer,
    /// or a ConnectionTracker derived object.
    /// \tparam TrackerType The type of the tracker, a shared_ptr, weak_ptr, or a pointer to the ConnectionTracker
    ///         derived object.
    /// \param tracker The tracker to bind to the slot.
    /// \see Connection::bind()
    template <class TrackerType>
    void bind(TrackerType tracker);

protected:
    /// Constructor.
    explicit SlotInterface() = default;

    /// Adds a tracker to the slot.
    virtual void addTracker(TrackerPtr&&) = 0;
};

/// The Connection holds a slot connected to a signal. It is a token to a receiver slot connected to
/// that signal.
class COMP_API Connection
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

    /// Disconnects the slot.
    void disconnect()
    {
        auto slot = m_slot.lock();
        if (!slot)
        {
            return;
        }
        slot->disconnect();
    }

    /// Returns the valid state of the connection.
    /// \return If the connection is valid, returns \e true, otherwise returns \e false. A connection is invalid when its
    /// source signal or its trackers are destroyed.
    operator bool() const
    {
        const auto slot = m_slot.lock();
        return slot && slot->isConnected();
    }

    /// Binds trackers to the slot. Trackers are objects that are used in a slot connected to a signal. To make sure the
    /// slot is only activated if all the objects used in the slot function are valid, bind trackers.
    ///
    /// A tracker is either
    /// - a pointer to a class derived from ConnectionTracker; the pointer is either a raw pointer, a shared pointer or an intrusive
    ///   pointer
    /// - a shared or weak pointer of an arbitrar object.
    ///
    /// You can use the same tracker to track multiple slots. To disconnect the slots tracked by a tracker that is derived
    /// from ConnectionTracker, call #ConnectionTracker::disconnectTrackedConnections(). Slots tracked by a shared pointer are disconnected only when
    /// that shared pointer is deleted.
    ///
    /// To untrack a slot in all its trackers, disconnect that slot.
    ///
    /// \tparam Trackers The types of the trackers to bind.
    /// \param trackers... The trackers to bind.
    /// \return This connection.
    /// \see SlotInterface::bind()
    template <class... Trackers>
    Connection& bind(Trackers... trackers);

    /// Returns the slot of the connection.
    /// \return The slot of the connection. If the connection is not valid, returns \e nullptr.
    SlotPtr get() const
    {
        return m_slot.lock();
    }

private:
    SlotWeakPtr m_slot;
};

/// To track the lifetime of a connection based on an arbitrary object that is not a smart pointer,
/// use this class. The class disconnects all tracked slots on destruction.
/// You can bind trackers to a connection using the #Connection::bind() method, by passing the pointer
/// to the ConnectionTracker object as argument of the method.
class COMP_API ConnectionTracker : public TrackerInterface
{
public:
    /// Destructor.
    ~ConnectionTracker()
    {
        disconnectTrackedConnections();
    }

    /// Attaches a \a connection to the trackable.
    void track(Connection connection) override
    {
        m_trackedConnections.push_back(connection);
    }

    /// Detaches the slot from the trackable.
    void untrack(Connection connection) override
    {
        erase_first(m_trackedConnections, connection);
    }

    /// Disconnects the attached slots. Call this method if you want to disconnect from the attached slot
    /// earlier than at the trackable destruction time.
    void disconnectTrackedConnections()
    {
        while (!m_trackedConnections.empty())
        {
            auto connection = m_trackedConnections.back();
            m_trackedConnections.pop_back();

            connection.disconnect();
        }
    }

protected:
    /// Constructor.
    explicit ConnectionTracker() = default;

private:
    /// This is valid as long as it exists.
    bool isValid() const final
    {
        return true;
    }
    vector<Connection> m_trackedConnections;
};

/********************************************************************************
 * Collectors
 */
/// Collectors are used with signals that have return type, and are used to collect the return values
/// of the slots conmnected to the signal.
///
/// You can create collectors deriving from this template class, and implement a \e handleResult function
/// with the following signature:
/// - \e {bool handleResult(Connection[, ReturnType [const&]])}
/// where \e Connection is the connection to the slot, and \e ReturnType is the return type of the slot.
/// You must specify the return type if the slot returns a non-void value. To stop the signal activation,
/// return \e false, otherwise return \e true.
template <class DerivedCollector>
class COMP_TEMPLATE_API Collector
{
    DerivedCollector* getSelf()
    {
        return static_cast<DerivedCollector*>(this);
    }

public:
    /// Constructor.
    explicit Collector() = default;

    /// Activates the slot and collects the return value of the slot. Your collector must implement
    /// a \e handleResult function to collect the results of the activated slot.
    /// \tparam SlotType
    /// \tparam ReturnType
    /// \tparam Arguments
    /// \param slot
    /// \param arguments
    /// \return If the collector succeeds, returns \e true, otherwise \e false. If the collector returns
    /// \e false, the signal activation breaks.
    template <class SlotType, typename ReturnType, typename... Arguments>
    bool collect(SlotType& slot, Arguments&&... arguments);
};

/// The default signal collector specialized for signals with void return type.
template <class T, typename Enable = void>
struct DefaultSignalCollector : public Collector<DefaultSignalCollector<T>>
{
    /// Returns the number of successful slot activations.
    size_t size() const
    {
        return callCount;
    }

    /// Handles the result. Slots with void return type only have the connection as argument.
    bool handleResult(Connection)
    {
        ++callCount;
        return true;
    }

private:
    size_t callCount = 0u;
};

/// The default signal collector specialized for signals with non-void return type.
template <class T>
struct DefaultSignalCollector<T, enable_if_t<!is_void_v<T>, void>> : public Collector<DefaultSignalCollector<T>>,
                                                                     public vector<T>
{
    /// Handles the result.
    bool handleResult(Connection, T result)
    {
        vector<T>::push_back(result);
        return true;
    }
};

/********************************************************************************
 * Concepts
 */

/// The Slot holds the invocable connected to a signal. The slot is a function, a function object, a method
/// or an other signal.
template <typename LockType, typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API SlotConcept : public SlotInterface, public Lockable<mutex>
{
public:
    /// Activates the slot with the arguments passed, and returns the slot's return value.
    ReturnType activate(Arguments&&...);

    /// Implements SlotInterface::isConnected().
    bool isConnected() const final;

    /// Implements SlotInterface::disconnect().
    void disconnect() final;

protected:
    /// To implement slot specific activation, override this method.
    virtual ReturnType activateOverride(Arguments&&...) = 0;

    /// Implements SlotInterface::addTracker().
    void addTracker(TrackerPtr&& tracker) final;

private:
    /// The container with the binded trackers.
    using TrackersContainer = vector<TrackerPtr>;

    /// The binded trackers.
    TrackersContainer m_trackers;
    atomic_bool m_isConnected = true;
};

/// The SignalConcept defines the concept of a signal. Defined as a lockable for convenience, holds the
/// connections of the signal.
template <class LockType, typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API SignalConcept : public Lockable<LockType>, public ConnectionTracker
{
public:
    using SlotType = SlotConcept<LockType, ReturnType, Arguments...>;
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

    /// Activates the signal with a specific \a Collector. Returns the collected results gathered from the
    /// activated slots by the collector type.
    /// \tparam Collector The collector used in emit.
    /// \param arguments The arguments to pass.
    template <class Collector = DefaultSignalCollector<ReturnType>>
    Collector operator()(Arguments... arguments);

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
    Connection connect(SignalConcept& receiver);

    /// Disconnects the \a connection passed as argument.
    /// \param connection The connection to disconnect. The connection is invalidated and removed from the signal.
    void disconnect(Connection connection);

protected:
    /// Constructor.
    explicit SignalConcept() = default;

    /// The container of the connections.
    FlagGuard m_emitGuard;

    /// The container with the connected slots.
    using SlotContainer = vector<SlotTypePtr>;
    SlotContainer m_slots;

private:
    atomic_bool m_isBlocked = false;
};

} // namespace comp

#endif // COMP_CONNECTION_HPP
