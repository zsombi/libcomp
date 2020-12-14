#ifndef SYWU_SIGNAL_IMPL_HPP
#define SYWU_SIGNAL_IMPL_HPP

#include <sywu/signal.hpp>
#include <sywu/extras.hpp>

namespace sywu
{

namespace
{

template <typename FunctionType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionSlot final : public SlotImpl<ReturnType, Arguments...>
{
    bool isValid() const override
    {
        return m_isValid.load();
    }

    void disconnect() override
    {
        m_isValid.store(false);
    }

    ReturnType activate(Arguments&&... args) override
    {
        return std::invoke(m_function, std::forward<Arguments>(args)...);
    }

public:
    explicit FunctionSlot(const FunctionType& function)
        : m_function(function)
    {
    }

private:
    FunctionType m_function;
    std::atomic_bool m_isValid = true;
};

template <class TargetObject, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MethodSlot final : public SlotImpl<ReturnType, Arguments...>
{
    bool isValid() const override
    {
        return !m_target.expired();
    }

    void disconnect() override
    {
        m_target.reset();
    }

    ReturnType activate(Arguments&&... arguments) override
    {
        auto slotHost = m_target.lock();
        SYWU_ASSERT(slotHost);
        return std::invoke(m_function, slotHost, std::forward<Arguments>(arguments)...);
    }

public:
    using FunctionType = ReturnType(TargetObject::*)(Arguments...);
    explicit MethodSlot(std::shared_ptr<TargetObject> target, const FunctionType& function)
        : m_target(target)
        , m_function(function)
    {        
    }

private:
    std::weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalSlot final : public SlotImpl<ReturnType, Arguments...>
{
    bool isValid() const override
    {
        return m_receiver != nullptr;
    }

    void disconnect() override
    {
        m_receiver = nullptr;
    }

    ReturnType activate(Arguments&&... arguments) override
    {
        if constexpr (std::is_void_v<ReturnType>)
        {
            std::invoke(*m_receiver, std::forward<Arguments>(arguments)...);
        }
        else
        {
            return std::invoke(*m_receiver, std::forward<Arguments>(arguments)...);
        }
    }

public:
    explicit SignalSlot(ReceiverSignal& receiver)
        : m_receiver(&receiver)
    {
    }

private:
    ReceiverSignal* m_receiver = nullptr;
};

} // namespace noname

template <class DerivedClass, typename ReturnType, typename... Arguments>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::~SignalConceptImpl()
{
    lock_guard lock(*this);

    auto invalidate = [](auto slot)
    {
        if (slot && slot->isValid())
        {
            slot->disconnect();
        }
    };
    utils::for_each(m_slots, invalidate);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
size_t SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::operator()(Arguments... arguments)
{
    if (isBlocked() || m_emitGuard.isLocked())
    {
        return 0u;
    }

    lock_guard guard(m_emitGuard);

    SlotContainer slots;
    {
        lock_guard lock(*this);
        slots = m_slots;
    }

    auto count = int(0);
    for (auto& slot : slots)
    {
        // The signal may get blocked within a connection, so bail out if that happens.
        if (isBlocked())
        {
            return count;
        }
        else
        {
            lock_guard lock(*slot);
            if (!slot->isValid())
            {
                relock_guard relock(*slot);
                disconnect(Connection(*this, slot));
            }
            else if (slot->isEnabled())
            {
                struct ConnectionSwapper
                {
                    Connection previousConnection;
                    explicit ConnectionSwapper(Connection connection)
                        : previousConnection(SignalConcept::currentConnection)
                    {
                        SignalConcept::currentConnection = std::move(connection);
                    }
                    ~ConnectionSwapper()
                    {
                        SignalConcept::currentConnection = previousConnection;
                    }
                };
                ConnectionSwapper backupConnection(Connection(*this, slot));
                relock_guard relock(*slot);
                slot->activate(std::forward<Arguments>(arguments)...);
                ++count;
            }
        }
    }

    return count;
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
Connection SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::addSlot(SlotPtr slot)
{
    auto slotActivator = std::dynamic_pointer_cast<SlotType>(slot);
    SYWU_ASSERT(slotActivator);
    lock_guard lock(*this);
    m_slots.push_back(slotActivator);
    return Connection(*this, m_slots.back());
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class FunctionType>
std::enable_if_t<std::is_member_function_pointer_v<FunctionType>, Connection>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(std::shared_ptr<typename traits::function_traits<FunctionType>::object> receiver, FunctionType method)
{
    using Object = typename traits::function_traits<FunctionType>::object;
    using SlotReturnType = typename traits::function_traits<FunctionType>::return_type;

    static_assert(
        traits::function_traits<FunctionType>::arity == 0 ||
        traits::function_traits<FunctionType>::template test_arguments<Arguments...>::value ||
        std::is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = std::make_shared<MethodSlot<Object, SlotReturnType, Arguments...>>(receiver, method);
    return addSlot(slot);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class FunctionType>
std::enable_if_t<!std::is_base_of_v<sywu::SignalConcept, FunctionType>, Connection>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(const FunctionType& function)
{
    using SlotReturnType = typename traits::function_traits<FunctionType>::return_type;
    static_assert(
        traits::function_traits<FunctionType>::arity == 0 ||
        traits::function_traits<FunctionType>::template test_arguments<Arguments...>::value ||
        std::is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = std::make_shared<FunctionSlot<FunctionType, SlotReturnType, Arguments...>>(function);
    return addSlot(slot);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class RDerivedClass, typename RReturnType, class... RArguments>
Connection SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>& receiver)
{
    using ReceiverSignal = SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>;
    static_assert(
        sizeof...(RArguments) == 0 ||
        std::is_same_v<std::tuple<Arguments...>, std::tuple<RArguments...>>,
        "incompatible signal signature");

    auto slot = std::make_shared<SignalSlot<ReceiverSignal, RReturnType, Arguments...>>(receiver);
    return addSlot(slot);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
void SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::disconnect(Connection connection)
{
    if (!connection.isValid())
    {
        return;
    }
    lock_guard guard(*this);
    auto slot = connection.m_slot.lock();
    connection.disconnect();
    utils::erase(m_slots, slot);
}

/******************************************************************************
 * MemberSignal
 */
template <class SignalHost, typename ReturnType, typename... Arguments>
MemberSignal<SignalHost, ReturnType(Arguments...)>::MemberSignal(SignalHost& signalHost)
    : m_host(signalHost)
{
}

template <class SignalHost, typename ReturnType, typename... Arguments>
size_t MemberSignal<SignalHost, ReturnType(Arguments...)>::operator()(Arguments... arguments)
{
    auto lockedHost = m_host.shared_from_this();
    SYWU_ASSERT(lockedHost);
    return BaseClass::operator()(std::forward<Arguments>(arguments)...);
}


} // namespace sywu

#endif // SIGNAL_IMPL_HPP
