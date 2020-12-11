#ifndef SIGNAL_IMPL_HPP
#define SIGNAL_IMPL_HPP

#include <sywu/signal.hpp>
#include <sywu/extras.hpp>

namespace sywu
{

namespace
{

/// The ConnectionConcept defines the activation concept of a connection.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API ConnectionConcept : public Connection
{
public:
    using ConnectionType = ConnectionConcept<ReturnType, Arguments...>;
    using InvokerType = ReturnType(*)(ConnectionType&, Arguments&&...);

    /// Activates the slot
    InvokerType invoker = nullptr;

protected:
    explicit ConnectionConcept(SignalConcept& signal, InvokerType invoker)
        : Connection(signal)
        , invoker(invoker)
    {
    }
};

template <typename FunctionType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionConnection final : public ConnectionConcept<ReturnType, Arguments...>
{
    using Base = ConnectionConcept<ReturnType, Arguments...>;

    static decltype(auto) activate(Base& connection, Arguments&&... args)
    {
        return static_cast<FunctionConnection&>(connection).m_function(std::forward<Arguments>(args)...);
    }

public:
    explicit FunctionConnection(SignalConcept& signal, const FunctionType& function)
        : Base(signal, &activate)
        , m_function(function)
    {
    }

private:
    FunctionType m_function;
};

template <class TargetObject, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MethodConnection final : public ConnectionConcept<ReturnType, Arguments...>
{
    using Base = ConnectionConcept<ReturnType, Arguments...>;

    static decltype(auto) activate(Base& connection, Arguments&&... arguments)
    {
        auto& self = static_cast<MethodConnection&>(connection);
        auto locked = self.m_target.lock();
        if (!locked)
        {
            abort();
        }
        return (*locked.*self.m_function)(std::forward<Arguments>(arguments)...);
    }

public:
    using FunctionType = ReturnType(TargetObject::*)(Arguments...);
    explicit MethodConnection(SignalConcept& signal, std::shared_ptr<TargetObject> target, const FunctionType& function)
        : Base(signal, &activate)
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

private:
    std::weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalConnection final : public ConnectionConcept<ReturnType, Arguments...>
{
    using Base = ConnectionConcept<ReturnType, Arguments...>;

    static decltype(auto) activate(Base& connection, Arguments&&... arguments)
    {
        auto& self = static_cast<SignalConnection&>(connection);
        if constexpr (std::is_void_v<ReturnType>)
        {
            (*self.m_receiver)(std::forward<Arguments>(arguments)...);
        }
        else
        {
            return (*self.m_receiver)(std::forward<Arguments>(arguments)...);
        }
    }

public:
    explicit SignalConnection(SignalConcept& sender, ReceiverSignal& receiver)
        : Base(sender, &activate)
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

private:
    ReceiverSignal* m_receiver = nullptr;
};

} // namespace noname

template <class DerivedClass, typename ReturnType, typename... Arguments>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::~SignalConceptImpl()
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

template <class DerivedClass, typename ReturnType, typename... Arguments>
size_t SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::operator()(Arguments... arguments)
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
        else
        {
            lock_guard lock(*connection);
            if (!connection->isValid())
            {
                if (connection->getSender())
                {
                    relock_guard relock(*connection);
                    disconnect(connection);
                }
            }
            else if (connection->isEnabled())
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
                ConnectionSwapper backupConnection(connection);
                relock_guard relock(*connection);
                auto connectionConcept = std::dynamic_pointer_cast<ConnectionConcept<ReturnType, Arguments...>>(connection);
                if (!connectionConcept)
                {
                    abort();
                }
                connectionConcept->invoker(*connectionConcept, std::forward<Arguments>(arguments)...);
                ++count;
            }
        }
    }

    return count;
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class SlotFunction>
std::enable_if_t<std::is_member_function_pointer_v<SlotFunction>, ConnectionPtr>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(std::shared_ptr<typename traits::function_traits<SlotFunction>::object> receiver, SlotFunction method)
{
    using Object = typename traits::function_traits<SlotFunction>::object;
    using SlotReturnType = typename traits::function_traits<SlotFunction>::return_type;

    static_assert(
        traits::function_traits<SlotFunction>::arity == 0 ||
        traits::function_traits<SlotFunction>::template test_arguments<Arguments...>::value ||
        std::is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    {
        lock_guard lock(*this);
        auto connection = std::make_shared<MethodConnection<Object, SlotReturnType, Arguments...>>(*this, receiver, method);
        m_connections.emplace_back(connection);
    }
    return m_connections.back();
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class SlotFunction>
std::enable_if_t<!std::is_base_of_v<sywu::SignalConcept, SlotFunction>, ConnectionPtr>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(const SlotFunction& slot)
{
    using SlotReturnType = typename traits::function_traits<SlotFunction>::return_type;
    static_assert(
        traits::function_traits<SlotFunction>::arity == 0 ||
        traits::function_traits<SlotFunction>::template test_arguments<Arguments...>::value ||
        std::is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    {
        lock_guard lock(*this);
        auto connection = std::make_shared<FunctionConnection<SlotFunction, SlotReturnType, Arguments...>>(*this, slot);
        m_connections.emplace_back(connection);
    }
    return m_connections.back();
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class RDerivedClass, typename RReturnType, class... RArguments>
ConnectionPtr SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>& receiver)
{
    using ReceiverSignal = SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>;
    static_assert(
        sizeof...(RArguments) == 0 ||
        std::is_same_v<std::tuple<Arguments...>, std::tuple<RArguments...>>,
        "incompatible signal signature");

    {
        lock_guard lock(*this);
        auto connection = std::make_shared<SignalConnection<ReceiverSignal, RReturnType, Arguments...>>(*this, receiver);
        m_connections.emplace_back(connection);
    }
    return m_connections.back();
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
void SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::disconnect(ConnectionPtr connection)
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
template <class SignalHost, typename ReturnType, typename... Arguments>
MemberSignal<SignalHost, ReturnType(Arguments...)>::MemberSignal(SignalHost& signalHost)
    : m_emitGuard(signalHost)
{
}

} // namespace sywu

#endif // SIGNAL_IMPL_HPP
