#ifndef COMP_TRACKER_HPP
#define COMP_TRACKER_HPP

#include <comp/config.hpp>
#include <comp/wrap/memory.hpp>
#include <comp/wrap/vector.hpp>

namespace comp
{

/// The %DeleteObserver gets notified when a watched object is deleted. The watched object must
/// derive from Notifier.
class COMP_API DeleteObserver : public comp::enable_shared_from_this<DeleteObserver>
{
public:
    /// The %Notifier tells DeleteObserver instances that watch the deletion of the object.
    class COMP_API Notifier
    {
        friend class DeleteObserver;
    public:
        /// Destructor.
        ~Notifier();

    private:
        using Observers = comp::vector<comp::weak_ptr<DeleteObserver>>;
        /// The observers that watch the deletion of this object.
        Observers m_observers;
    };

    /// Destructor.
    virtual ~DeleteObserver() = default;

    /// Watch the deletion of the \a object.
    /// \param object The object whose deletion to watch.
    void watch(Notifier& object);

    /// Unwatches the deletion of the \a object.
    /// \param object The object to unwatch.
    void unwatch(Notifier& object);

protected:
    /// Tells the observer that an \a object is deleted.
    /// \param object The object that is deleted.
    virtual void notifyDeleted(Notifier& object) = 0;
};

} // comp

#endif // COMP_TRACKER_HPP
