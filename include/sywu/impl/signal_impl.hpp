#ifndef SYWU_SIGNAL_IMPL_HPP
#define SYWU_SIGNAL_IMPL_HPP

#include <sywu/concept/connection_impl.hpp>
#include <sywu/signal.hpp>
#include <sywu/wrap/memory.hpp>
#include <sywu/wrap/exception.hpp>
#include <sywu/wrap/functional.hpp>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/vector.hpp>

namespace sywu
{

namespace
{

template <typename FunctionType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionSlot final : public SlotImpl<ReturnType, Arguments...>
{
    using Base = SlotImpl<ReturnType, Arguments...>;

    void deactivateOverride() override
    {
    }

    ReturnType activateOverride(Arguments&&... args) override
    {
        return invoke(m_function, forward<Arguments>(args)...);
    }

public:
    explicit FunctionSlot(SignalConcept& sender, const FunctionType& function)
        : Base(sender)
        , m_function(function)
    {
    }

private:
    FunctionType m_function;
};

template <class TargetObject, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MethodSlot final : public SlotImpl<ReturnType, Arguments...>
{
    using Base = SlotImpl<ReturnType, Arguments...>;

    bool isActiveOverride() const override
    {
        return !m_target.expired();
    }

    void deactivateOverride() override
    {
        m_target.reset();
    }

    ReturnType activateOverride(Arguments&&... arguments) override
    {
        auto slotHost = m_target.lock();
        if (!slotHost)
        {
            throw bad_slot();
        }

        return invoke(m_function, slotHost, forward<Arguments>(arguments)...);
    }

public:
    using FunctionType = ReturnType(TargetObject::*)(Arguments...);
    explicit MethodSlot(SignalConcept& sender, shared_ptr<TargetObject> target, const FunctionType& function)
        : Base(sender)
        , m_target(target)
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
    using Base = SlotImpl<ReturnType, Arguments...>;

    void deactivateOverride() override
    {
        m_receiver = nullptr;
    }

    ReturnType activateOverride(Arguments&&... arguments) override
    {
        if (!m_receiver)
        {
            throw bad_slot();
        }

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
    explicit SignalSlot(SignalConcept& sender, ReceiverSignal& receiver)
        : Base(sender)
        , m_receiver(&receiver)
    {
    }

private:
    ReceiverSignal* m_receiver = nullptr;
};

struct ConnectionSwapper
{
    Connection previousConnection;
    explicit ConnectionSwapper(SlotPtr slot)
        : previousConnection(ActiveConnection::connection)
    {
        ActiveConnection::connection = Connection(slot);
    }
    ~ConnectionSwapper()
    {
        ActiveConnection::connection = previousConnection;
    }
};

} // namespace noname

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
        lock_guard lock(*slot);

        try
        {
            if (!slot->getSignal())
            {
                // The slot is already disconnected from the signal, most likely due to the signal deletion.
                continue;
            }
            ConnectionSwapper backupConnection(slot);
            relock_guard relock(*slot);
            static_pointer_cast<SlotType>(slot)->activate(forward<Arguments>(arguments)...);
            ++count;
        }
        catch (const bad_weak_ptr&)
        {
            relock_guard relock(*slot);
            disconnect(Connection(slot));
        }
        catch (const bad_slot&)
        {
            relock_guard relock(*slot);
            disconnect(Connection(slot));
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
    return Connection(m_slots.back());
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

    auto slot = make_shared<Slot, MethodSlot<Object, SlotReturnType, Arguments...>>(*this, receiver, method);
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

    auto slot = make_shared<Slot, FunctionSlot<FunctionType, SlotReturnType, Arguments...>>(*this, function);
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

    auto slot = make_shared<Slot, SignalSlot<ReceiverSignal, RReturnType, Arguments...>>(*this, receiver);
    receiver.attach(slot);
    return addSlot(slot);
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
