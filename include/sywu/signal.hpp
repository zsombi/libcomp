#ifndef SYWU_SIGNAL_HPP
#define SYWU_SIGNAL_HPP

#include <sywu/config.hpp>
#include <sywu/concept/signal.hpp>
#include <sywu/wrap/mutex.hpp>

namespace sywu
{

template <typename ReturnType, typename... Arguments>
class Signal;

/// The signal template. Use this template to define a signal with a signature.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class SYWU_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConcept<mutex, ReturnType, Arguments...>
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
class SYWU_TEMPLATE_API MemberSignal<SignalHost, ReturnType(Arguments...)> : public SignalConcept<mutex, ReturnType, Arguments...>
{
    using BaseClass = SignalConcept<mutex, ReturnType, Arguments...>;
    SignalHost& m_host;

public:
    /// Constructor
    explicit MemberSignal(SignalHost& signalHost);

    /// Emit override for method signals.
    size_t operator()(Arguments... arguments);
};

} // namespace sywu

#endif // SYWU_SIGNAL_HPP
