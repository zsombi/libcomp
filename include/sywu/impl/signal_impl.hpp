#ifndef SYWU_SIGNAL_IMPL_HPP
#define SYWU_SIGNAL_IMPL_HPP

#include <sywu/signal.hpp>
#include <sywu/concept/signal_concept_impl.hpp>

namespace sywu
{

/******************************************************************************
 * MemberSignal
 */
template <class SignalHost, typename ReturnType, typename... Arguments>
MemberSignal<SignalHost, ReturnType(Arguments...)>::MemberSignal(SignalHost& signalHost)
    : m_host(signalHost)
{
}

template <class SignalHost, typename ReturnType, typename... Arguments>
template <class Collector>
Collector MemberSignal<SignalHost, ReturnType(Arguments...)>::operator()(Arguments... arguments)
{
    auto lockedHost = m_host.shared_from_this();
    SYWU_ASSERT(lockedHost);
    auto collector = BaseClass::template operator()<Collector>(forward<Arguments>(arguments)...);
    return move(collector);
}

} // namespace sywu

#endif // SIGNAL_IMPL_HPP
