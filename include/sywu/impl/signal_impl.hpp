#ifndef SYWU_SIGNAL_IMPL_HPP
#define SYWU_SIGNAL_IMPL_HPP

#include <sywu/wrap/memory.hpp>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/functional.hpp>
#include <sywu/signal.hpp>
#include <sywu/wrap/vector.hpp>
#include <sywu/concept/connection_impl.hpp>

namespace sywu
{

namespace
{

template <typename FunctionType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionSlot final : public SlotImpl<ReturnType, Arguments...>
{
    bool isValidOverride() const override
    {
        return m_isValid.load();
    }

    void disconnectOverride() override
    {
        m_isValid.store(false);
    }

    ReturnType activateOverride(Arguments&&... args) override
    {
        return invoke(m_function, forward<Arguments>(args)...);
    }

public:
    explicit FunctionSlot(const FunctionType& function)
        : m_function(function)
    {
    }

private:
    FunctionType m_function;
    atomic_bool m_isValid = true;
};

template <class TargetObject, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MethodSlot final : public SlotImpl<ReturnType, Arguments...>
{
    bool isValidOverride() const override
    {
        return !m_target.expired();
    }

    void disconnectOverride() override
    {
        m_target.reset();
    }

    ReturnType activateOverride(Arguments&&... arguments) override
    {
        auto slotHost = m_target.lock();
        SYWU_ASSERT(slotHost);
        return invoke(m_function, slotHost, forward<Arguments>(arguments)...);
    }

public:
    using FunctionType = ReturnType(TargetObject::*)(Arguments...);
    explicit MethodSlot(shared_ptr<TargetObject> target, const FunctionType& function)
        : m_target(target)
        , m_function(function)
    {        
    }

private:
    weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalSlot final : public SlotImpl<ReturnType, Arguments...>
{
    bool isValidOverride() const override
    {
        return m_receiver != nullptr;
    }

    void disconnectOverride() override
    {
        m_receiver = nullptr;
    }

    ReturnType activateOverride(Arguments&&... arguments) override
    {
        if constexpr (is_void_v<ReturnType>)
        {
            invoke(*m_receiver, forward<Arguments>(arguments)...);
        }
        else
        {
            return invoke(*m_receiver, forward<Arguments>(arguments)...);
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
    for_each(m_slots, invalidate);
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
            else
            {
                struct ConnectionSwapper
                {
                    Connection previousConnection;
                    explicit ConnectionSwapper(Connection connection)
                        : previousConnection(ConnectionHelper::currentConnection)
                    {
                        ConnectionHelper::currentConnection = move(connection);
                    }
                    ~ConnectionSwapper()
                    {
                        ConnectionHelper::currentConnection = previousConnection;
                    }
                };
                ConnectionSwapper backupConnection(Connection(*this, slot));
                relock_guard relock(*slot);
                static_pointer_cast<SlotType>(slot)->activate(forward<Arguments>(arguments)...);
                ++count;
            }
        }
    }

    return count;
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
Connection SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::addSlot(SlotPtr slot)
{
    auto slotActivator = dynamic_pointer_cast<SlotType>(slot);
    SYWU_ASSERT(slotActivator);
    lock_guard lock(*this);
    m_slots.push_back(slotActivator);
    return Connection(*this, m_slots.back());
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<is_member_function_pointer_v<FunctionType>, Connection>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(shared_ptr<typename function_traits<FunctionType>::object> receiver, FunctionType method)
{
    using Object = typename function_traits<FunctionType>::object;
    using SlotReturnType = typename function_traits<FunctionType>::return_type;

    static_assert(
        function_traits<FunctionType>::template test_arguments<Arguments...>::value &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<Slot, MethodSlot<Object, SlotReturnType, Arguments...>>(receiver, method);
    return addSlot(slot);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<!is_base_of_v<sywu::SignalConcept, FunctionType>, Connection>
SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(const FunctionType& function)
{
    using SlotReturnType = typename function_traits<FunctionType>::return_type;
    static_assert(
        function_traits<FunctionType>::template test_arguments<Arguments...>::value &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<Slot, FunctionSlot<FunctionType, SlotReturnType, Arguments...>>(function);
    return addSlot(slot);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
template <class RDerivedClass, typename RReturnType, class... RArguments>
Connection SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::connect(SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>& receiver)
{
    using ReceiverSignal = SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>;
    static_assert(
        is_same_v<tuple<Arguments...>, tuple<RArguments...>>,
        "incompatible signal signature");

    auto slot = make_shared<Slot, SignalSlot<ReceiverSignal, RReturnType, Arguments...>>(receiver);
    return addSlot(slot);
}

template <class DerivedClass, typename ReturnType, typename... Arguments>
void SignalConceptImpl<DerivedClass, ReturnType, Arguments...>::disconnect(Connection connection)
{
    lock_guard guard(*this);
    auto slot = connection.m_slot.lock();
    if (!slot)
    {
        return;
    }
    connection.disconnect();
    erase(m_slots, slot);
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
    return BaseClass::operator()(forward<Arguments>(arguments)...);
}


} // namespace sywu

#endif // SIGNAL_IMPL_HPP
