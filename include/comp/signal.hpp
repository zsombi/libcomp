#ifndef COMP_SIGNAL_HPP
#define COMP_SIGNAL_HPP

#include <comp/wrap/mutex.hpp>
#include <comp/concept/signal.hpp>
#include <comp/concept/signal_impl.hpp>

namespace comp
{

template <typename TRet, typename... TArgs>
class Signal;

/// Specialization for a standalone signal. Use this template to define a signal with a signature.
/// \tparam ReturnType The return type of the signal.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <typename ReturnType, typename... Arguments>
class COMP_TEMPLATE_API Signal<ReturnType(Arguments...)> : public SignalConceptImpl<ReturnType, Arguments...>
{
public:
    /// Constructor.
    explicit Signal() = default;
};

/// Use this template variant to create a member signal on a class that derives from enable_shared_from_this<>.
/// The signal makes sure the sender object is kept alive for the signal activation time. Do not use this signal
/// declaration if you activate the signal in the destructor of the signal host.
/// \tparam SignalHost The class on which the member signal is defined.
/// \tparam ReturnType The return type of the signal.
/// \tparam Arguments The arguments of the signal, which is the signature of the signal.
template <class SignalHost, typename ReturnType, typename... Arguments>
class Signal<ReturnType(SignalHost::*)(Arguments...)> : public SignalConceptImpl<ReturnType, Arguments...>
{
    using BaseClass = SignalConceptImpl<ReturnType, Arguments...>;
    SignalHost& m_host;

public:
    /// Constructor.
    explicit Signal(SignalHost& host)
        : m_host(host)
    {
    }

    /// Activation override for member signals.
    int operator()(Arguments... args)
    {
        auto null = NullCollector<ReturnType>();
        return this->emit(null, comp::forward<Arguments>(args)...);
    }

    /// Emit override for member signals.
    template <class Collector = NullCollector<ReturnType>>
    int emit(Collector& collector, Arguments... arguments)
    {
        auto lockedHost = m_host.shared_from_this();
        COMP_ASSERT(lockedHost);
        return BaseClass::emit(collector, comp::forward<Arguments>(arguments)...);
    }
};

} // namespace comp

#endif // COMP_SIGNAL_HPP
