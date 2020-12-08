#ifndef SIGNAL_IMPL_HPP
#define SIGNAL_IMPL_HPP

#include <sywu/signal.hpp>
#include <sywu/extras.hpp>

namespace sywu
{

ConnectionPtr SignalConcept::currentConnection;

void Connection::disconnect()
{
    lock_guard lock(*this);
    disconnectOverride();
    m_sender = nullptr;
}

namespace
{

/// The ConnectionConcept defines the activation concept of a connection.
template <typename... Arguments>
class SYWU_TEMPLATE_API ConnectionConcept : public Connection
{
public:
    /// Activates the slot
    bool activate(Arguments&&... arguments)
    {
        lock_guard guard(*this);
        if (!isValid() || !isEnabled())
        {
            return false;
        }

        {
            struct ConnectionSwapper
            {
                ConnectionPtr previousConnection;
                explicit ConnectionSwapper(ConnectionPtr connection)
                    : previousConnection(SignalConcept::currentConnection)
                {
                    SignalConcept::currentConnection = connection;
                }
                ~ConnectionSwapper()
                {
                    SignalConcept::currentConnection = previousConnection;
                }
            };
            ConnectionSwapper backupConnection(shared_from_this());
            relock_guard relock(*this);
            activateOverride(std::forward<Arguments>(arguments)...);
        }
        return true;
    }

protected:
    explicit ConnectionConcept(Signal<Arguments...>& signal)
        : Connection(signal)
    {
    }

    virtual void activateOverride(Arguments&&... arguments) = 0;
};

template <typename FunctionType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionConnection final : public ConnectionConcept<Arguments...>
{
    using Base = ConnectionConcept<Arguments...>;
public:
    explicit FunctionConnection(Signal<Arguments...>& signal, const FunctionType& function)
        : Base(signal)
        , m_function(function)
    {
    }

protected:
    void activateOverride(Arguments&&... arguments) override
    {
        m_function(std::forward<Arguments>(arguments)...);
    }

private:
    FunctionType m_function;
};

template <class TargetObject, typename... Arguments>
class SYWU_TEMPLATE_API MethodConnection final : public ConnectionConcept<Arguments...>
{
    using Base = ConnectionConcept<Arguments...>;
public:
    using FunctionType = void(TargetObject::*)(Arguments...);
    explicit MethodConnection(Signal<Arguments...>& signal, std::shared_ptr<TargetObject> target, const FunctionType& function)
        : Base(signal)
        , m_target(target)
        , m_function(function)
    {
    }

protected:
    bool isValidOverride() const override
    {
        return !m_target.expired();
    }

    void disconnectOverride() override
    {
        m_target.reset();
    }

    void activateOverride(Arguments&&... arguments) override
    {
        auto locked = m_target.lock();
        if (!locked)
        {
            return;
        }
        (*locked.*m_function)(std::forward<Arguments>(arguments)...);
    }

private:
    std::weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename... Arguments>
class SYWU_TEMPLATE_API SignalConnection final : public ConnectionConcept<Arguments...>
{
    using Base = ConnectionConcept<Arguments...>;

public:
    explicit SignalConnection(Signal<Arguments...>& sender, ReceiverSignal& receiver)
        : Base(sender)
        , m_receiver(&receiver)
    {
    }

protected:
    bool isValidOverride() const override
    {
        return m_receiver != nullptr;
    }

    void disconnectOverride() override
    {
        m_receiver = nullptr;
    }

    void activateOverride(Arguments&&... arguments) override
    {
        (*m_receiver)(std::forward<Arguments>(arguments)...);
    }

private:
    ReceiverSignal* m_receiver = nullptr;
};

} // namespace noname

SignalConcept::~SignalConcept()
{
    while (!m_connections.empty())
    {
        disconnect(m_connections.back());
    }
}

void SignalConcept::disconnect(ConnectionPtr connection)
{
    if (!connection)
    {
        return;
    }
    lock_guard guard(*this);
    utils::erase(m_connections, connection);
    connection->disconnect();
}


template <typename... Arguments>
size_t Signal<Arguments...>::operator()(Arguments... arguments)
{
    if (isBlocked() || m_emitGuard > 0)
    {
        return 0u;
    }

    RefCounter guard(m_emitGuard);

    ConnectionContainer connections;
    {
        lock_guard lock(*this);
        connections = m_connections;
    }

    auto count = int(0);
    for (auto& connection : connections)
    {
        // The signal may get blocked within a connection, so bail out if that happens.
        if (isBlocked())
        {
            return count;
        }

        auto connectionConcept = std::dynamic_pointer_cast<ConnectionConcept<Arguments...>>(connection);
        if (!connectionConcept)
        {
            abort();
        }

        if (connectionConcept->activate(std::forward<Arguments>(arguments)...))
        {
            ++count;
        }
        else if (!connectionConcept->isValid() && connectionConcept->getSender())
        {
            disconnect(connection);
        }
    }

    return count;
}

template <typename... Arguments>
template <class SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionPtr>
Signal<Arguments...>::connect(std::shared_ptr<typename traits::function_traits<SlotFunction>::object> receiver, SlotFunction method)
{
    using Object = typename traits::function_traits<SlotFunction>::object;

    static_assert(
        traits::function_traits<SlotFunction>::arity == 0 ||
        traits::function_traits<SlotFunction>::template test_arguments<Arguments...>::value,
        "Incompatible slot signature");

    {
        lock_guard lock(*this);
        auto connection = std::make_shared<MethodConnection<Object, Arguments...>>(*this, receiver, method);
        m_connections.emplace_back(connection);
    }
    return m_connections.back();
}

template <typename... Arguments>
template <class SlotFunction>
std::enable_if_t<!std::is_base_of_v<sywu::SignalConcept, SlotFunction>, ConnectionPtr>
Signal<Arguments...>::connect(const SlotFunction& slot)
{
    static_assert(
        traits::function_traits<SlotFunction>::arity == 0 ||
        traits::function_traits<SlotFunction>::template test_arguments<Arguments...>::value,
        "Incompatible slot signature");

    {
        lock_guard lock(*this);
        auto connection = std::make_shared<FunctionConnection<SlotFunction, Arguments...>>(*this, slot);
        m_connections.emplace_back(connection);
    }
    return m_connections.back();
}

template <typename... Arguments>
template <class... SignalArguments>
ConnectionPtr Signal<Arguments...>::connect(Signal<SignalArguments...>& receiver)
{
    using ReceiverSignal = Signal<SignalArguments...>;
    static_assert(
        sizeof...(SignalArguments) == 0 ||
        std::is_same_v<std::tuple<Arguments...>, std::tuple<SignalArguments...>>,
        "incompatible signal signature");

    {
        lock_guard lock(*this);
        auto connection = std::make_shared<SignalConnection<ReceiverSignal, Arguments...>>(*this, receiver);
        m_connections.emplace_back(connection);
    }
    return m_connections.back();
}

} // namespace sywu

#endif // SIGNAL_IMPL_HPP
