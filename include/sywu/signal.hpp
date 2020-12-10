#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <config.hpp>
#include <sywu/guards.hpp>
#include <sywu/type_traits.hpp>

namespace sywu
{

class SignalConcept;

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionWeakPtr = std::weak_ptr<Connection>;

/// The Connection holds a slot connected to a signal.
class SYWU_API Connection : public Lockable, public std::enable_shared_from_this<Connection>
{
    friend class SignalConcept;

public:
    /// Destructor.
    virtual ~Connection() = default;
    /// Disconnect the connection from the signal.
    void disconnect()
    {
        lock_guard lock(*this);
        disconnectOverride();
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
    bool isValid() const
    {
        return (m_sender != nullptr) && isValidOverride();
    }
    /// Returns the enabled state of a connection.
    /// \return If the connection is enabled, returns \e true, otherwise returns \e false.
    bool isEnabled() const
    {
        return m_isEnabled.load();
    }
    /// Sets the enabled state of a connection.
    /// \param enable The enabled state to set for the connection.
    void setEnabled(bool enable)
    {
        m_isEnabled.store(enable);
    }

protected:
    /// Constructs the connection with the \a sender signal.
    explicit Connection(SignalConcept& sender)
        : m_sender(&sender)
    {
    }

    /// To define connection specific validity, override this method.
    /// \return If the override is valid, return \e true, otherwise \e false.
    virtual bool isValidOverride() const
    {
        return true;
    }

    /// To define connection specific disconnect operations, override this method.
    virtual void disconnectOverride()
    {
    }

private:
    SignalConcept* m_sender = nullptr;
    std::atomic_bool m_isEnabled = true;
};

/// The SignalConcept defines the concept of the signals. Defined as a lockable for convenience, holds the
/// connections of the signal.
class SYWU_API SignalConcept
{
    DISABLE_COPY_OR_MOVE(SignalConcept);

public:
    /// The current connection. Use this member to access the connection that holds the connected
    /// slot that is activated by the signal.
    static inline ConnectionPtr currentConnection;

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
    /// The container of the connections.
    using ConnectionContainer = std::vector<ConnectionPtr>;
    /// Hidden default constructor.
    explicit SignalConcept() = default;

    ConnectionContainer m_connections;

private:
    std::atomic_bool m_isBlocked = false;
};

/// Signal concept implementation.
template <class DerivedClass, typename... Arguments>
class SYWU_TEMPLATE_API SignalConceptImpl : public Lockable, public SignalConcept
{
    DerivedClass* getSelf()
    {
        return static_cast<DerivedClass*>(this);
    }

public:
    /// Destructor.
    ~SignalConceptImpl();

    /// Activates the signal with the given arguments.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections invoked.
    size_t operator()(Arguments... arguments);

    /// Connects a \a method of a \a receiver to this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return Returns the shared pointer to the connection.
    template <class SlotFunction>
    std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionPtr>
    connect(std::shared_ptr<typename traits::function_traits<SlotFunction>::object> receiver, SlotFunction method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param slot The function, functor or lambda to connect.
    /// \return Returns the shared pointer to the connection.
    template <class SlotFunction>
    std::enable_if_t<!std::is_base_of_v<sywu::SignalConcept, SlotFunction>, ConnectionPtr>
    connect(const SlotFunction& slot);

    /// Creates a connection between this signal and a \a receiver signal.
    /// \param receiver The receiver signal connected to this signal.
    /// \return Returns the shared pointer to the connection.
    template <class TLockableClass, class... SignalArguments>
    ConnectionPtr connect(SignalConceptImpl<TLockableClass, SignalArguments...>& receiver);

    /// Disconnects the \a connection passed as argument.
    /// \param connection The connection to disconnect. The connection is invalidated and removed from the signal.
    void disconnect(ConnectionPtr connection);

protected:
    explicit SignalConceptImpl() = default;
};

template <typename ReturnType, typename... Arguments>
class Signal;

/// The signal template. Use this template to define a signal with a signature.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConceptImpl<Signal<ReturnType(Arguments...)>, Arguments...>
{
    friend class SignalConceptImpl<Signal<ReturnType(Arguments...)>, Arguments...>;
    BoolLock m_emitGuard;

public:
    /// Constructor.
    explicit Signal() = default;
};

template <class SignalHost, typename ReturnType, typename... Arguments>
class MemberSignal;
/// Use this template to create a member signal on a class that derives from std::enable_shared_from_this<>.
/// \tparam SignalHost The class on which the member signal is defined.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <class SignalHost, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MemberSignal<SignalHost, ReturnType(Arguments...)> : public SignalConceptImpl<MemberSignal<SignalHost, ReturnType(Arguments...)>, Arguments...>
{
    friend class SignalConceptImpl<MemberSignal<SignalHost, ReturnType(Arguments...)>, Arguments...>;
    SharedPtrLock<SignalHost> m_emitGuard;

public:
    /// Constructor
    explicit MemberSignal(SignalHost& signalHost);
};

} // namespace sywu

#endif // SIGNAL_HPP
