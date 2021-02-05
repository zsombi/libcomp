#ifndef COMP_CONNECTION_HPP
#define COMP_CONNECTION_HPP

#include <comp/config.hpp>
#include <comp/concept/core/signal_impl.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/vector.hpp>
#include <comp/wrap/type_traits.hpp>
#include <comp/wrap/function_traits.hpp>
#include <comp/utility/tracker.hpp>

namespace comp
{

// Forward declarations.
using SlotPtr = shared_ptr<core::Slot<mutex>>;
using SlotWeakPtr = weak_ptr<core::Slot<mutex>>;

class COMP_API Connection : public core::Connection
{
public:

    Connection() = default;

    /// Constructs the connection with a \a slot.
    Connection(SlotPtr slot)
        : core::Connection(slot)
    {
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
    /// from ConnectionTracker, call #ConnectionTracker::clearTrackables(). Slots tracked by a shared pointer are disconnected only when
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

private:
    /// Binds a \a tracker object to a \a slot. The tracker object is either a shared pointer, a weak pointer,
    /// a ConnectionTracker object, or a shared pointer to a ConnectionTracker.
    template <class TrackerType>
    void bindOne(SlotPtr slot, TrackerType tracker);
};

/// To track the lifetime of a connection based on an arbitrary object that is not a smart pointer,
/// use this class. The class disconnects all tracked slots on destruction.
/// You can bind trackers to a connection using the #Connection::bind() method, by passing the pointer
/// to the ConnectionTracker object as argument of the method.
using ConnectionTracker = Tracker<Connection>;

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
template <typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API SlotConcept : public core::Slot<mutex>
{
    using Base = core::Slot<mutex>;
public:
    /// Activates the slot with the arguments passed, and returns the slot's return value.
    ReturnType activate(Arguments&&...);

protected:
    /// Constructor.
    explicit SlotConcept(core::Signal& signal)
        : Base(signal)
    {
    }

    /// To implement slot specific activation, override this method.
    virtual ReturnType activateOverride(Arguments&&...) = 0;
};

/// The SignalConcept defines the concept of a signal. Defined as a lockable for convenience, holds the
/// connections of the signal.
template <typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API SignalConcept : public Lockable<mutex>, public core::Signal, public ConnectionTracker
{
public:
    using SlotType = SlotConcept<ReturnType, Arguments...>;
    using SlotTypePtr = shared_ptr<SlotType>;
    using SignalConceptType = SignalConcept<ReturnType, Arguments...>;

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
    void disconnect(core::Connection connection) override;

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
