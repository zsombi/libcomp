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
/// \tparam ReturnType The return type of the signal.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConcept<ReturnType, Arguments...>
{
public:
    /// Constructor.
    explicit Signal() = default;
};

/// Use this template variant to create a member signal on a class that derives from enable_shared_from_this<>.
/// The signal makes sure the sender object is kept alive for the signal activation time. Do not use this signal
/// declaration if the signal is activated in the destructor.
/// \tparam SignalHost The class on which the member signal is defined.
/// \tparam ReturnType The return type of the signal.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <class SignalHost, typename ReturnType, typename... Arguments>
class Signal<ReturnType(SignalHost::*)(Arguments...)> : public SignalConcept<ReturnType, Arguments...>
{
    using BaseClass = SignalConcept<ReturnType, Arguments...>;
    SignalHost& m_host;

public:
    /// Constructor.
    explicit Signal(SignalHost& host)
        : m_host(host)
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
