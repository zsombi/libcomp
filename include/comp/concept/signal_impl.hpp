#ifndef COMP_SIGNAL_CONCEPT_IMPL_HPP
#define COMP_SIGNAL_CONCEPT_IMPL_HPP

#include <comp/concept/signal.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/exception.hpp>
#include <comp/wrap/functional.hpp>
#include <comp/wrap/utility.hpp>
#include <comp/wrap/vector.hpp>

namespace comp
{

template <typename TRet, typename... TArgs>
SignalConceptImpl<TRet, TArgs...>::SlotType::SlotType(SignalConcept& signal)
    : ConnectionConcept(signal)
{
}

template <typename TRet, typename... TArgs>
template <class TCollector>
void SignalConceptImpl<TRet, TArgs...>::SlotType::activate(TCollector& collector, TArgs&&... args)
{
    if constexpr (comp::is_void_v<TRet>)
    {
        activateOverride(comp::forward<TArgs>(args)...);
    }
    else
    {
        auto ret = activateOverride(comp::forward<TArgs>(args)...);
        collector.collect(ret);
    }
}


template <typename TRet, typename... TArgs>
int SignalConceptImpl<TRet, TArgs...>::operator()(TArgs... args)
{
    auto null = NullCollector<TRet>();
    return emit(null, comp::forward<TArgs>(args)...);
}

template <typename TRet, typename... TArgs>
template <class Collector>
int SignalConceptImpl<TRet, TArgs...>::emit(Collector& collector, TArgs... args)
{
    if (isBlocked() || m_emitGuard.isLocked())
    {
        return -1;
    }

    comp::lock_guard guard(m_emitGuard);

    ConnectionContainer connections;
    {
        comp::lock_guard lock(*this);
        comp::erase_if(m_connections, [](auto& slot) { return !slot || !slot->isValid(); });
        connections = m_connections;
    }

    int result = 0;
    for (auto& connection : connections)
    {
        auto slot = comp::dynamic_pointer_cast<SlotType>(connection);
        COMP_ASSERT(slot);
        comp::lock_guard lock(*slot);

        try
        {
            if (!slot->isValid())
            {
                continue;
            }

            ++result;
            comp::relock_guard re(*slot);
            slot->activate(collector, comp::forward<TArgs>(args)...);
        }
        catch (const comp::bad_slot&)
        {
            comp::relock_guard re(*slot);
            disconnect(*slot);
        }
        catch (const comp::bad_weak_ptr&)
        {
            comp::relock_guard re(*slot);
            disconnect(*slot);
        }
    }

    return result;
}


namespace
{

// A connection to a function or a static method.
template <typename Function, typename TRet, typename... TArgs>
class FunctionConnection final : public SignalConceptImpl<TRet, TArgs...>::SlotType
{
    Function m_function;

public:
    explicit FunctionConnection(SignalConcept& signal, const Function& function)
        : SignalConceptImpl<TRet, TArgs...>::SlotType(signal)
        , m_function(function)
    {
    }

protected:
    TRet activateOverride(TArgs&&... args) override
    {
        if constexpr (function_traits<Function>::arity == 0u)
        {
            return comp::invoke(m_function, comp::forward<TArgs>(args)...);
        }
        else if constexpr (is_same_v<ConnectionPtr, typename function_traits<Function>::template argument<0u>::type>)
        {
            auto connection = comp::dynamic_pointer_cast<SignalConcept::ConnectionConcept>(this->shared_from_this());
            return comp::invoke(m_function, connection, comp::forward<TArgs>(args)...);
        }
        else
        {
            return comp::invoke(m_function, comp::forward<TArgs>(args)...);
        }
    }
};

// A connection to a method.
template <class Target, typename Method, typename TRet, typename... TArgs>
class MethodConnection final : public SignalConceptImpl<TRet, TArgs...>::SlotType
{
    comp::weak_ptr<Target> m_target;
    Method m_method;

public:
    explicit MethodConnection(SignalConcept& signal, comp::shared_ptr<Target> target, const Method& method)
        : SignalConceptImpl<TRet, TArgs...>::SlotType(signal)
        , m_target(target)
        , m_method(method)
    {
    }

protected:
    TRet activateOverride(TArgs&&... args) override
    {
        auto slot = m_target.lock();
        if (!slot)
        {
            throw comp::bad_slot();
        }
        if constexpr (function_traits<Method>::arity == 0u)
        {
            return comp::invoke(m_method, slot, comp::forward<TArgs>(args)...);
        }
        else if constexpr (is_same_v<ConnectionPtr, typename function_traits<Method>::template argument<0u>::type>)
        {
            auto connection = comp::dynamic_pointer_cast<SignalConcept::ConnectionConcept>(this->shared_from_this());
            return comp::invoke(m_method, slot, connection, comp::forward<TArgs>(args)...);
        }
        else
        {
            return comp::invoke(m_method, slot, comp::forward<TArgs>(args)...);
        }
    }
};

template <typename Ret>
struct LastRet
{
    Ret m_lastRet = Ret();
    void collect(Ret& result)
    {
        m_lastRet = result;
    }
};
template <>
struct LastRet<void>
{
};

// A connection to an other signal with similar signature.
template <class Receiver, typename TRet, typename... TArgs>
class SignalConnection final : public SignalConceptImpl<TRet, TArgs...>::SlotType
{
    Receiver* m_receiver = nullptr;

public:
    explicit SignalConnection(SignalConcept& sender, Receiver& receiver)
        : SignalConceptImpl<TRet, TArgs...>::SlotType(sender)
        , m_receiver(&receiver)
    {
    }

protected:
    TRet activateOverride(TArgs&&... args)
    {
        auto collector = LastRet<TRet>();
        m_receiver->emit(collector, comp::forward<TArgs>(args)...);
        if constexpr (!comp::is_void_v<TRet>)
        {
            return collector.m_lastRet;
        }
    }
};

}

template <typename TRet, typename... TArgs>
template <class FunctionType>
enable_if_t<!is_base_of_v<SignalConcept, FunctionType>, ConnectionPtr>
SignalConceptImpl<TRet, TArgs...>::connect(const FunctionType& function)
{
    using SlotReturnType = typename function_traits<FunctionType>::return_type;
    static_assert(
        (function_traits<FunctionType>::template is_same_args<TArgs...> || function_traits<FunctionType>::template is_same_args<ConnectionPtr, TArgs...>) &&
        is_same_v<TRet, SlotReturnType>,
        "Incompatible slot signature");

    auto connection = make_shared<FunctionConnection<FunctionType, TRet, TArgs...>>(*this, function);
    addConnection(connection);
    return connection;
}

template <typename TRet, typename... TArgs>
template <class Method>
enable_if_t<is_member_function_pointer_v<Method>, ConnectionPtr>
SignalConceptImpl<TRet, TArgs...>::connect(shared_ptr<typename function_traits<Method>::object> receiver, Method method)
{
    using Object = typename function_traits<Method>::object;
    using SlotReturnType = typename function_traits<Method>::return_type;

    static_assert(
        (function_traits<Method>::template is_same_args<TArgs...>  || function_traits<Method>::template is_same_args<ConnectionPtr, TArgs...>) &&
        is_same_v<TRet, SlotReturnType>,
        "Incompatible slot signature");

    auto connection = make_shared<MethodConnection<Object, Method, TRet, TArgs...>>(*this, receiver, method);
    addConnection(connection);
    if constexpr (is_base_of_v<DeleteObserver::Notifier, Object>)
    {
        connection->watch(*receiver);
    }
    return connection;
}

template <typename TRet, typename... TArgs>
template <typename ReceiverResult, typename... TReceiverArgs>
ConnectionPtr SignalConceptImpl<TRet, TArgs...>::connect(SignalConceptImpl<ReceiverResult, TReceiverArgs...>& receiver)
{
    using ReceiverSignal = SignalConceptImpl<ReceiverResult, TReceiverArgs...>;

    static_assert(
        sizeof...(TReceiverArgs) == 0 ||
        is_same_v<std::tuple<TArgs...>, tuple<TReceiverArgs...>>,
        "incompatible signal signature");

    auto connection = make_shared<SignalConnection<ReceiverSignal, TRet, TArgs...>>(*this, receiver);
    addConnection(connection);
    connection->watch(receiver);
    return connection;
}

} // namespace comp

#endif // COMP_SIGNAL_CONCEPT_IMPL_HPP
