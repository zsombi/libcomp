#ifndef COMP_SIGNAL_CONCEPT_IMPL_HPP
#define COMP_SIGNAL_CONCEPT_IMPL_HPP

#include <comp/concept/signal.hpp>
#include <comp/concept/slot_concept_impl.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/exception.hpp>
#include <comp/wrap/functional.hpp>
#include <comp/wrap/utility.hpp>
#include <comp/wrap/vector.hpp>

namespace comp
{

namespace
{

template <typename FunctionType, typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API FunctionSlot final : public SlotConcept<ReturnType, Arguments...>
{
    ReturnType activateOverride(Arguments&&... args) override
    {
        if constexpr (function_traits<FunctionType>::arity == 0u)
        {
            return invoke(m_function, forward<Arguments>(args)...);
        }
        else if constexpr (is_same_v<Connection, typename function_traits<FunctionType>::template argument<0u>::type>)
        {
            return invoke(m_function, Connection(this->shared_from_this()), forward<Arguments>(args)...);
        }
        else
        {
            return invoke(m_function, forward<Arguments>(args)...);
        }
    }

public:
    explicit FunctionSlot(core::Signal& signal, const FunctionType& function)
        : SlotConcept<ReturnType, Arguments...>(signal)
        , m_function(function)
    {
    }

private:
    FunctionType m_function;
};

template <class TargetObject, typename FunctionType, typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API MethodSlot final : public SlotConcept<ReturnType, Arguments...>
{
    ReturnType activateOverride(Arguments&&... arguments) override
    {
        auto slotHost = m_target.lock();
        if (!slotHost)
        {
            throw bad_slot();
        }

        if constexpr (function_traits<FunctionType>::arity == 0u)
        {
            return invoke(m_function, slotHost, forward<Arguments>(arguments)...);
        }
        else if constexpr (is_same_v<Connection, typename function_traits<FunctionType>::template argument<0u>::type>)
        {
            return invoke(m_function, slotHost, Connection(this->shared_from_this()), forward<Arguments>(arguments)...);
        }
        else
        {
            return invoke(m_function, slotHost, forward<Arguments>(arguments)...);
        }
    }

public:
    explicit MethodSlot(core::Signal& signal, shared_ptr<TargetObject> target, const FunctionType& function)
        : SlotConcept<ReturnType, Arguments...>(signal)
        , m_target(target)
        , m_function(function)
    {
    }

private:
    weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API SignalSlot final : public SlotConcept<ReturnType, Arguments...>
{
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
    explicit SignalSlot(core::Signal& signal, ReceiverSignal& receiver)
        : SlotConcept<ReturnType, Arguments...>(signal)
        , m_receiver(&receiver)
    {
    }

private:
    ReceiverSignal* m_receiver = nullptr;
};

} // namespace noname

template <class DerivedCollector>
template <class SlotType, typename ReturnType, typename... Arguments>
bool Collector<DerivedCollector>::collect(SlotType& slot, Arguments&&... arguments)
{
    if constexpr (is_void_v<ReturnType>)
    {
        slot.activate(forward<Arguments>(arguments)...);
        return getSelf()->handleResult(Connection(slot.shared_from_this()));
    }
    else
    {
        auto result = slot.activate(forward<Arguments>(arguments)...);
        return getSelf()->handleResult(Connection(slot.shared_from_this()), result);
    }
}


template <typename ReturnType, typename... Arguments>
SignalConcept<ReturnType, Arguments...>::~SignalConcept()
{
    lock_guard lock(*this);

    while (!m_slots.empty())
    {
        relock_guard relock(*this);
        disconnect(Connection(m_slots.back()));
    }
}

template <typename ReturnType, typename... Arguments>
template <class Collector>
Collector SignalConcept<ReturnType, Arguments...>::operator()(Arguments... arguments)
{
    auto context = Collector();

    if (isBlocked() || m_emitGuard.isLocked())
    {
        return context;
    }

    lock_guard guard(m_emitGuard);

    SlotContainer slots;
    {
        lock_guard lock(*this);
        // Remove disconnected slots.
        erase_if(m_slots, [](auto& slot) { return !slot || !slot->isConnected(); });
        // Copyt the slots now.
        slots = m_slots;
    }

    for (auto& s : slots)
    {
        auto slot = static_pointer_cast<SlotType>(s);
        lock_guard lock(*slot);

        try
        {
            if (!slot->isConnected())
            {
                // The slot is already disconnected from the signal, most likely due to this signal deletion.
                continue;
            }
            relock_guard relock(*slot);
            if (!context.template collect<SlotType, ReturnType, Arguments...>(*slot, forward<Arguments>(arguments)...))
            {
                break;
            }
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

    return context;
}

template <typename ReturnType, typename... Arguments>
Connection SignalConcept<ReturnType, Arguments...>::addSlot(SlotPtr slot)
{
    auto slotActivator = dynamic_pointer_cast<SlotType>(slot);
    COMP_ASSERT(slotActivator);
    lock_guard lock(*this);
    m_slots.push_back(slotActivator);
    return Connection(m_slots.back());
}

template <typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<is_member_function_pointer_v<FunctionType>, Connection>
SignalConcept<ReturnType, Arguments...>::connect(shared_ptr<typename function_traits<FunctionType>::object> receiver, FunctionType method)
{
    using Object = typename function_traits<FunctionType>::object;
    using SlotReturnType = typename function_traits<FunctionType>::return_type;

    static_assert(
        (function_traits<FunctionType>::template is_same_args<Arguments...> || function_traits<FunctionType>::template is_same_args<Connection, Arguments...>) &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<core::Slot<mutex>, MethodSlot<Object, FunctionType, SlotReturnType, Arguments...>>(*this, receiver, method);
    return addSlot(slot).bind(receiver);
}

template <typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<!is_base_of_v<SignalConcept<ReturnType, Arguments...>, FunctionType>, Connection>
SignalConcept<ReturnType, Arguments...>::connect(const FunctionType& function)
{
    using SlotReturnType = typename function_traits<FunctionType>::return_type;
    static_assert(
        (function_traits<FunctionType>::template is_same_args<Arguments...> || function_traits<FunctionType>::template is_same_args<Connection, Arguments...>) &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<core::Slot<mutex>, FunctionSlot<FunctionType, SlotReturnType, Arguments...>>(*this, function);
    return addSlot(slot);
}

template <typename ReturnType, typename... Arguments>
Connection SignalConcept<ReturnType, Arguments...>::connect(SignalConcept& receiver)
{
    using ReceiverSignal = SignalConcept;
    auto slot = make_shared<core::Slot<mutex>, SignalSlot<ReceiverSignal, ReturnType, Arguments...>>(*this, receiver);
    receiver.track(Connection(slot));
    return addSlot(slot);
}

template <typename ReturnType, typename... Arguments>
void SignalConcept<ReturnType, Arguments...>::disconnect(Connection connection)
{
    auto slot = connection.get();
    if (!slot)
    {
        return;
    }
    else
    {
        lock_guard lock(*this);
        auto it = find(m_slots, slot);
        if (it == m_slots.end())
        {
            return;
        }
        erase(m_slots, slot);
    }
    connection.disconnect();
}

} // namespace comp

#endif // COMP_SIGNAL_CONCEPT_IMPL_HPP
