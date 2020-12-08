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
template <typename... Arguments>
class Signal;

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
    void disconnect();

    /// Returns the sender signal of the connection.
    /// \return The sender signal. If the connection is invalid, returns \e nullptr.
    SignalConcept* getSender() const
    {
        return m_sender;
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
class SYWU_API SignalConcept : public Lockable
{
    friend class Connection;
    DISABLE_COPY_OR_MOVE(SignalConcept);

public:
    /// The current connection. Use this member to access the connection that holds the connected
    /// slot that is activated by the signal.
    static ConnectionPtr currentConnection;

    /// Destructor
    ~SignalConcept();

    /// Disconnects the \a connection passed as argument.
    /// \param connection The connection to disconnect. The connection is invalidated and removed from the signal.
    void disconnect(ConnectionPtr connection);

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

/// The signal template. Use this template to define a signal with a signature.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename... Arguments>
class SYWU_TEMPLATE_API Signal : public SignalConcept
{
    AtomicRefCounted<int> m_emitGuard = 0;

public:
    /// Constructor.
    explicit Signal() = default;

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
    template <class... SignalArguments>
    ConnectionPtr connect(Signal<SignalArguments...>& receiver);
};

} // namespace sywu

#endif // SIGNAL_HPP
