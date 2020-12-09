#ifndef SIGNAL_IMPL_HPP
#define SIGNAL_IMPL_HPP

#include <sywu/signal.hpp>
#include <sywu/extras.hpp>

namespace sywu
{

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
    explicit ConnectionConcept(SignalConcept& signal)
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
    explicit FunctionConnection(SignalConcept& signal, const FunctionType& function)
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
    explicit MethodConnection(SignalConcept& signal, std::shared_ptr<TargetObject> target, const FunctionType& function)
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
    explicit SignalConnection(SignalConcept& sender, ReceiverSignal& receiver)
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

template <class DerivedClass, typename... Arguments>
SignalConceptImpl<DerivedClass, Arguments...>::~SignalConceptImpl()
{
    lock_guard lock(*this);

    auto invalidate = [](auto connection)
    {
        if (connection && connection->isValid())
        {
            connection->disconnect();
        }
    };
    utils::for_each(m_connections, invalidate);
}

template <class DerivedClass, typename... Arguments>
size_t SignalConceptImpl<DerivedClass, Arguments...>::operator()(Arguments... arguments)
{
    if (isBlocked() || getSelf()->m_emitGuard.isLocked())
    {
        return 0u;
    }

    lock_guard guard(getSelf()->m_emitGuard);

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

template <class DerivedClass, typename... Arguments>
template <class SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionPtr>
SignalConceptImpl<DerivedClass, Arguments...>::connect(std::shared_ptr<typename traits::function_traits<SlotFunction>::object> receiver, SlotFunction method)
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

template <class DerivedClass, typename... Arguments>
template <class SlotFunction>
std::enable_if_t<!std::is_base_of_v<sywu::SignalConcept, SlotFunction>, ConnectionPtr>
SignalConceptImpl<DerivedClass, Arguments...>::connect(const SlotFunction& slot)
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

template <class DerivedClass, typename... Arguments>
template <class TDerivedClass, class... SignalArguments>
ConnectionPtr SignalConceptImpl<DerivedClass, Arguments...>::connect(SignalConceptImpl<TDerivedClass, SignalArguments...>& receiver)
{
    using ReceiverSignal = SignalConceptImpl<TDerivedClass, SignalArguments...>;
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

template <class DerivedClass, typename... Arguments>
void SignalConceptImpl<DerivedClass, Arguments...>::disconnect(ConnectionPtr connection)
{
    if (!connection)
    {
        return;
    }
    lock_guard guard(*this);
    utils::erase(m_connections, connection);
    connection->disconnect();
}

/******************************************************************************
 * MemberSignal
 */
template <class SignalHost, typename... Arguments>
MemberSignal<SignalHost, Arguments...>::MemberSignal(SignalHost& signalHost)
    : m_emitGuard(signalHost)
{
}

} // namespace sywu

#endif // SIGNAL_IMPL_HPP
