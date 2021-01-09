#ifndef SYWU_SIGNAL_CONCEPT_IMPL_HPP
#define SYWU_SIGNAL_CONCEPT_IMPL_HPP

#include <sywu/concept/signal.hpp>
#include <sywu/concept/slot_concept_impl.hpp>
#include <sywu/wrap/memory.hpp>
#include <sywu/wrap/exception.hpp>
#include <sywu/wrap/functional.hpp>
#include <sywu/wrap/utility.hpp>
#include <sywu/wrap/vector.hpp>

namespace sywu
{

namespace
{

template <typename FunctionType, typename LockType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API FunctionSlot final : public SlotConcept<LockType, ReturnType, Arguments...>
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
    explicit FunctionSlot(const FunctionType& function)
        : m_function(function)
    {
    }

private:
    FunctionType m_function;
};

template <class TargetObject, typename FunctionType, typename LockType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MethodSlot final : public SlotConcept<LockType, ReturnType, Arguments...>
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
    explicit MethodSlot(shared_ptr<TargetObject> target, const FunctionType& function)
        : m_target(target)
        , m_function(function)
    {
    }

private:
    weak_ptr<TargetObject> m_target;
    FunctionType m_function;
};

template <typename ReceiverSignal, typename LockType, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalSlot final : public SlotConcept<LockType, ReturnType, Arguments...>
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
    explicit SignalSlot(ReceiverSignal& receiver)
        : m_receiver(&receiver)
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

template <class LockType, typename ReturnType, typename... Arguments>
SignalConcept<LockType, ReturnType, Arguments...>::~SignalConcept()
{
    lock_guard lock(*this);

    while (!m_slots.empty())
    {
        auto slot = m_slots.back();
        m_slots.pop_back();
        slot->disconnect();
    }
}

template <class LockType, typename ReturnType, typename... Arguments>
template <class Collector>
Collector SignalConcept<LockType, ReturnType, Arguments...>::operator()(Arguments... arguments)
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

template <class LockType, typename ReturnType, typename... Arguments>
Connection SignalConcept<LockType, ReturnType, Arguments...>::addSlot(SlotPtr slot)
{
    auto slotActivator = dynamic_pointer_cast<SlotType>(slot);
    SYWU_ASSERT(slotActivator);
    lock_guard lock(*this);
    m_slots.push_back(slotActivator);
    return Connection(m_slots.back());
}

template <class LockType, typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<is_member_function_pointer_v<FunctionType>, Connection>
SignalConcept<LockType, ReturnType, Arguments...>::connect(shared_ptr<typename function_traits<FunctionType>::object> receiver, FunctionType method)
{
    using Object = typename function_traits<FunctionType>::object;
    using SlotReturnType = typename function_traits<FunctionType>::return_type;

    static_assert(
        (function_traits<FunctionType>::template is_same_args<Arguments...> || function_traits<FunctionType>::template is_same_args<Connection, Arguments...>) &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<SlotInterface, MethodSlot<Object, FunctionType, LockType, SlotReturnType, Arguments...>>(receiver, method);
    slot->bind(receiver);
    return addSlot(slot);
}

template <class LockType, typename ReturnType, typename... Arguments>
template <class FunctionType>
enable_if_t<!is_base_of_v<SignalConcept<LockType, ReturnType, Arguments...>, FunctionType>, Connection>
SignalConcept<LockType, ReturnType, Arguments...>::connect(const FunctionType& function)
{
    using SlotReturnType = typename function_traits<FunctionType>::return_type;
    static_assert(
        (function_traits<FunctionType>::template is_same_args<Arguments...> || function_traits<FunctionType>::template is_same_args<Connection, Arguments...>) &&
        is_same_v<ReturnType, SlotReturnType>,
        "Incompatible slot signature");

    auto slot = make_shared<SlotInterface, FunctionSlot<FunctionType, LockType, SlotReturnType, Arguments...>>(function);
    return addSlot(slot);
}

template <class LockType, typename ReturnType, typename... Arguments>
Connection SignalConcept<LockType, ReturnType, Arguments...>::connect(SignalConcept& receiver)
{
    using ReceiverSignal = SignalConcept;
    auto slot = make_shared<SlotInterface, SignalSlot<ReceiverSignal, LockType, ReturnType, Arguments...>>(receiver);
    receiver.track(slot);
    return addSlot(slot);
}

template <class LockType, typename ReturnType, typename... Arguments>
void SignalConcept<LockType, ReturnType, Arguments...>::disconnect(Connection connection)
{
    lock_guard lock(*this);
    auto slot = connection.get();
    connection.disconnect();
    erase(m_slots, slot);
}

} // namespace sywu

#endif // SYWU_SIGNAL_CONCEPT_IMPL_HPP
