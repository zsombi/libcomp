#ifndef COMP_SIGNAL_HPP
#define COMP_SIGNAL_HPP

#include <comp/wrap/mutex.hpp>
#include <comp/concept/signal.hpp>
#include <comp/concept/signal_concept_impl.hpp>

namespace comp
{

template <typename ReturnType, typename... Arguments>
class Signal;

/// The signal template. Use this template to define a signal with a signature.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConcept<mutex, ReturnType, Arguments...>
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
class COMP_TEMPLATE_API MemberSignal<SignalHost, ReturnType(Arguments...)> : public SignalConcept<mutex, ReturnType, Arguments...>
{
    using BaseClass = SignalConcept<mutex, ReturnType, Arguments...>;
    SignalHost& m_host;

public:
    /// Constructor
    explicit MemberSignal(SignalHost& signalHost)
        : m_host(signalHost)
    {
    }

    /// Emit override for method signals.
    template <class Collector = DefaultSignalCollector<ReturnType>>
    Collector operator()(Arguments... arguments)
    {
        auto lockedHost = m_host.shared_from_this();
        COMP_ASSERT(lockedHost);
        auto collector = BaseClass::template operator()<Collector>(forward<Arguments>(arguments)...);
        return collector;
    }
};

} // namespace comp

#endif // COMP_SIGNAL_HPP
