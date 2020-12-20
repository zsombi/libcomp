#ifndef SYWU_SIGNAL_HPP
#define SYWU_SIGNAL_HPP

#include <sywu/wrap/atomic.hpp>
#include <sywu/wrap/memory.hpp>
#include <sywu/wrap/type_traits.hpp>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/vector.hpp>

#include <sywu/config.hpp>
#include <sywu/connection.hpp>

namespace sywu
{

/// The SignalConcept defines the concept of the signals. Defined as a lockable for convenience, holds the
/// connections of the signal.
class SYWU_API SignalConcept
{
    SYWU_DISABLE_COPY_OR_MOVE(SignalConcept);

public:
    /// The current connection. Use this member to access the connection that holds the connected
    /// slot that is activated by the signal.
    static inline Connection currentConnection;

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

protected:
    /// Hidden default constructor.
    explicit SignalConcept() = default;

private:
    atomic_bool m_isBlocked = false;
};

/// Signal concept implementation.
template <class DerivedClass, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalConceptImpl : public Lockable, public SignalConcept
{
    DerivedClass* getSelf()
    {
        return static_cast<DerivedClass*>(this);
    }

public:
    using SlotType = SlotImpl<ReturnType, Arguments...>;

    /// Destructor.
    ~SignalConceptImpl();

    /// Activates the signal with the given arguments.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections invoked.
    size_t operator()(Arguments... arguments);

    /// Adds a \a slot to the signal.
    /// \param slot The slot to add to the signal.
    /// \return The connection token with teh signal and the slot.
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
    enable_if_t<!is_base_of_v<sywu::SignalConcept, FunctionType>, Connection>
    connect(const FunctionType& function);

    /// Creates a connection between this signal and a \a receiver signal.
    /// \param receiver The receiver signal connected to this signal.
    /// \return Returns the shared pointer to the connection.
    template <class RDerivedClass, typename RReturnType, class... RArguments>
    Connection connect(SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>& receiver);

    /// Disconnects the \a connection passed as argument.
    /// \param connection The connection to disconnect. The connection is invalidated and removed from the signal.
    void disconnect(Connection connection);

protected:
    explicit SignalConceptImpl() = default;

    /// The container of the connections.
    using SlotContainer = vector<shared_ptr<SlotType>>;
    SlotContainer m_slots;
    FlagGuard m_emitGuard;
};

template <typename ReturnType, typename... Arguments>
class Signal;

/// The signal template. Use this template to define a signal with a signature.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConceptImpl<Signal<ReturnType(Arguments...)>, ReturnType, Arguments...>
{
public:
    /// Constructor.
    explicit Signal() = default;
};

template <class SignalHost, typename ReturnType, typename... Arguments>
class MemberSignal;
/// Use this template to create a member signal on a class that derives from enable_shared_from_this<>. If you intend to
/// emit the signal from the destructor of that class, use Signal<>.
/// \tparam SignalHost The class on which the member signal is defined.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <class SignalHost, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MemberSignal<SignalHost, ReturnType(Arguments...)> : public SignalConceptImpl<MemberSignal<SignalHost, ReturnType(Arguments...)>, ReturnType, Arguments...>
{
    using BaseClass = SignalConceptImpl<MemberSignal<SignalHost, ReturnType(Arguments...)>, ReturnType, Arguments...>;
    SignalHost& m_host;

public:
    /// Constructor
    explicit MemberSignal(SignalHost& signalHost);

    /// Emit override for method signals.
    size_t operator()(Arguments... arguments);

};

} // namespace sywu

#endif // SYWU_SIGNAL_HPP
