#ifndef COMP_CONNECTION_HPP
#define COMP_CONNECTION_HPP

#include <comp/config.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/mutex.hpp>
#include <comp/wrap/vector.hpp>
#include <comp/wrap/type_traits.hpp>
#include <comp/wrap/function_traits.hpp>
#include <comp/utility/lockable.hpp>
#include <comp/utility/tracker.hpp>

namespace comp
{

/// Signal concept.
class COMP_API SignalConcept : public comp::Lockable<comp::mutex>, public comp::DeleteObserver::Notifier
{
public:
    /// Destructor.
    ~SignalConcept();

    /// The Connection to a signal.
    class COMP_API Connection : public comp::Lockable<comp::mutex>, public comp::DeleteObserver
    {
    public:
        /// Returns whether the connection object is valid. A connection object is valid when it
        /// is connected to a signal.
        bool isValid() const;

        /// Disconnects the connection from the signal it is connected to.
        void disconnect();

    protected:
        /// Constructor.
        explicit Connection(SignalConcept& signal);

        /// To perform connection specific cleanup when a connection disconnects, implement
        /// this method.
        virtual void disconnectOverride();

    private:
        /// Overrides DeleteObserver::notifyDeleted().
        void notifyDeleted(Notifier&) override;

        SignalConcept* m_signal = nullptr;
    };

    /// Returns whether the signal activation is blocked.
    /// \return If teh signal activation is blocked, returns \e true, otherwise \e false.
    bool isBlocked() const;

    /// Sets the signal activation blocked state.
    /// \param blocked To block the signal activation, pass \e true as argument, otherwise
    ///        pass \e false.
    void setBlocked(bool blocked);

    /// Adds a connection to the signal.
    /// \param connection The connection to add.
    void addConnection(comp::shared_ptr<Connection> connection);
    /// Disconnects a connection.
    /// \param connection The connection to disconnect.
    void disconnect(Connection& connection);

    /// Disconnects all the connections of a signal.
    void disconnect();
protected:

    /// Removes a connection from the container.
    /// \param connection The connection to remove.
    void removeConnection(Connection& connection);

    using ConnectionContainer = comp::vector<comp::shared_ptr<Connection>>;
    /// The container with the signal connections.
    ConnectionContainer m_connections;
    /// Signal re-activation guard.
    comp::FlagGuard m_emitGuard;

private:
    /// The blocked state of the signal.
    comp::atomic_bool m_isBlocked = false;
};

/// The pointer to a signal connection.
using ConnectionPtr = comp::shared_ptr<SignalConcept::Connection>;

/// The default result collector of a signal.
template <typename TRet>
struct NullCollector
{
    /// Null signal result collector.
    void collect(TRet&)
    {
    }
};

/// Specialization for void signals.
template <>
struct NullCollector<void>
{
};

/// The signal concept implementation. To declare signals, use one of the Signal template
/// specializations.
/// \tparam TRet The return type of the signal
/// \tparam TArgs The variadic arguments of the signal.
template <typename TRet, typename... TArgs>
class COMP_TEMPLATE_API SignalConceptImpl : public SignalConcept
{
public:
    /// The slot, the connection of a signal to a function, a lambda, a method or an other signal.
    class COMP_TEMPLATE_API SlotType : public SignalConcept::Connection
    {
    public:
        /// Activates the slot, and collects the results using the \a collector.
        /// \tparam TCollector The collector
        template <class TCollector>
        void activate(TCollector& collector, TArgs&&... args);

    protected:
        /// Constructor, creates a slot with a signal.
        explicit SlotType(SignalConcept& signal);

        /// The activation overridable.
        /// \param TArgs The arguments to pass to the slot.
        /// \return The return value of the slot.
        virtual TRet activateOverride(TArgs&&... args) = 0;
    };

    /// Constructor.
    explicit SignalConceptImpl() = default;

    /// Activates the signal, invokes the connected slots.
    /// \param args The arguments to pass to the slots.
    /// \return The number of connections activated.
    int operator()(TArgs... args);

    /// Activates the signal with a specific \a collector. Th ecollector collects the results of
    /// the activated slots.
    /// \tparam Collector The collector type which collects the slot results.
    /// \param collector The collector which collects the slot results.
    /// \param arguments The arguments to pass to the slots.
    /// \return The number of connections activated. A return value of -1 means the signal is blocked,
    ///         or the signal is re-activated.
    template <class Collector = NullCollector<TRet>>
    int emit(Collector& collector, TArgs... args);

    /// Connects a \a method of a \a receiver to this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return Returns the shared pointer to the connection.
    template <class Method>
    enable_if_t<is_member_function_pointer_v<Method>, ConnectionPtr>
    connect(shared_ptr<typename function_traits<Method>::object> receiver, Method method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param slot The function, functor or lambda to connect.
    /// \return Returns the shared pointer to the connection.
    template <class FunctionType>
    enable_if_t<!is_base_of_v<SignalConcept, FunctionType>, ConnectionPtr>
    connect(const FunctionType& function);

    /// Creates a connection between this signal and a \a receiver signal.
    /// \param receiver The receiver signal connected to this signal.
    /// \return Returns the shared pointer to the connection.
    template <typename ReceiverResult, typename... TReceiverArgs>
    ConnectionPtr connect(SignalConceptImpl<ReceiverResult, TReceiverArgs...>& receiver);
};


} // namespace comp

#endif // COMP_CONNECTION_HPP
