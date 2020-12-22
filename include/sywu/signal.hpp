#ifndef SYWU_SIGNAL_HPP
#define SYWU_SIGNAL_HPP

#include <atomic>
#include <sywu/wrap/memory.hpp>
#include <type_traits>
#include <utility>
#include <vector>

#include <sywu/config.hpp>
#include <sywu/concept/signal.hpp>
#include <sywu/wrap/mutex.hpp>
#include <sywu/wrap/type_traits.hpp>
#include <sywu/wrap/function_traits.hpp>

namespace sywu
{

/// Signal concept implementation.
template <class DerivedClass, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API SignalConceptImpl : public SignalConcept
{
    DerivedClass* getSelf()
    {
        return static_cast<DerivedClass*>(this);
    }

public:
    using SlotType = SlotImpl<ReturnType, Arguments...>;
    using SlotTypePtr = shared_ptr<SlotType>;

    /// Activates the signal with the given arguments.
    /// \param arguments... The variadic arguments passed.
    /// \return The number of connections invoked.
    size_t operator()(Arguments... arguments);

    /// Adds a \a slot to the signal.
    /// \param slot The slot to add to the signal.
    /// \return The connection token with the signal and the slot.
    Connection addSlot(SlotPtr slot);

    /// Connects a \a method of a \a receiver to this signal.
    /// \param receiver The receiver of the connection.
    /// \param method The method to connect.
    /// \return Returns the shared pointer to the connection.
    template <class FunctionType>
    enable_if_t<is_member_function_pointer_v<FunctionType>, Connection>
    connect(shared_ptr<typename function_traits<FunctionType>::object> receiver, FunctionType method);

    /// Connects a \a function, or a lambda to this signal.
    /// \param slot The function, functor or lambda to connect.
    /// \return Returns the shared pointer to the connection.
    template <class FunctionType>
    enable_if_t<!is_base_of_v<sywu::SignalConcept, FunctionType>, Connection>
    connect(const FunctionType& function);

    /// Creates a connection between this signal and a \a receiver signal.
    /// \param receiver The receiver signal connected to this signal.
    /// \return Returns the shared pointer to the connection.
    template <class RDerivedClass, typename RReturnType, class... RArguments>
    Connection connect(SignalConceptImpl<RDerivedClass, RReturnType, RArguments...>& receiver);

protected:
    explicit SignalConceptImpl() = default;

    /// The container of the connections.
    FlagGuard m_emitGuard;
};

template <typename ReturnType, typename... Arguments>
class Signal;

/// The signal template. Use this template to define a signal with a signature.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConceptImpl<Signal<ReturnType(Arguments...)>, ReturnType, Arguments...>
{
public:
    /// Constructor.
    explicit Signal() = default;
};

template <class SignalHost, typename ReturnType, typename... Arguments>
class MemberSignal;
/// Use this template to create a member signal on a class that derives from enable_shared_from_this<>.
/// \tparam SignalHost The class on which the member signal is defined.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <class SignalHost, typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API MemberSignal<SignalHost, ReturnType(Arguments...)> : public SignalConceptImpl<MemberSignal<SignalHost, ReturnType(Arguments...)>, ReturnType, Arguments...>
{
    using BaseClass = SignalConceptImpl<MemberSignal<SignalHost, ReturnType(Arguments...)>, ReturnType, Arguments...>;
    SignalHost& m_host;

public:
    /// Constructor
    explicit MemberSignal(SignalHost& signalHost);

    /// Emit override for method signals.
    size_t operator()(Arguments... arguments);

};

} // namespace sywu

#endif // SYWU_SIGNAL_HPP
