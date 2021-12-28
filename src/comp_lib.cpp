#include <comp/signal.hpp>
#include <comp/utility/tracker.hpp>

namespace comp
{

DeleteObserver::Notifier::~Notifier()
{
    auto visitor = [this](auto& observer)
    {
        auto tracked = observer.lock();
        if (tracked)
        {
            tracked->notifyDeleted(*this);
        }
    };
    comp::for_each(m_observers.cbegin(), m_observers.cend(), visitor);
}

void DeleteObserver::watch(Notifier& object)
{
    object.m_observers.emplace_back(shared_from_this());
}

void DeleteObserver::unwatch(Notifier& object)
{
    auto predicate = [this](auto& weak)
    {
        auto strong = weak.lock();
        return (strong.get() == this);
    };
    erase_if(object.m_observers, predicate);
}

SignalConcept::ConnectionConcept::ConnectionConcept(SignalConcept& signal)
    : m_signal(&signal)
{
}

void SignalConcept::ConnectionConcept::disconnectOverride()
{
}

void SignalConcept::ConnectionConcept::notifyDeleted(Notifier&)
{
    disconnect();
}

bool SignalConcept::ConnectionConcept::isValid() const
{
    return m_signal != nullptr;
}

void SignalConcept::ConnectionConcept::disconnect()
{
    if (!isValid())
    {
        return;
    }

    disconnectOverride();

    auto tmp = m_signal;
    m_signal = nullptr;
    if (tmp)
    {
        tmp->removeConnection(*this);
    }
}


SignalConcept::~SignalConcept()
{
    setBlocked(true);
    disconnect();
}

bool SignalConcept::isBlocked() const
{
    return m_isBlocked;
}
void SignalConcept::setBlocked(bool blocked)
{
    m_isBlocked = blocked;
}

void SignalConcept::addConnection(ConnectionPtr connection)
{
    comp::lock_guard lock(*this);
    m_connections.emplace_back(connection);
}

void SignalConcept::disconnect(ConnectionConcept& connection)
{
    removeConnection(connection);
}

void SignalConcept::disconnect()
{
    comp::lock_guard lock(*this);
    while (!m_connections.empty())
    {
        comp::relock_guard relock(*this);
        removeConnection(*m_connections.back());
    }
}

void SignalConcept::removeConnection(ConnectionConcept& connection)
{
    comp::lock_guard lock(*this);
    auto predicate = [&connection](auto conn)
    {
        return conn.get() == &connection;
    };
    auto it = find_if(m_connections.begin(), m_connections.end(), predicate);

    if (it != m_connections.end())
    {
        auto keepAlive = *it;
        m_connections.erase(it);

        if (keepAlive)
        {
            comp::relock_guard relock(*this);
            keepAlive->disconnect();
        }
    }
}

}
