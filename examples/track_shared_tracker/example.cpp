#include <comp/signal.hpp>
#include <iostream>

class Object : public comp::ConnectionTracker, public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

int main()
{
    comp::Signal<void()> signal;

    auto object = comp::make_shared<comp::ConnectionTracker, Object>();

    // Note: capture shared objects as weak pointer!
    auto slot = [weakObject = comp::weak_ptr<comp::ConnectionTracker>(object)]()
    {
        auto locked = weakObject.lock();
        if (!locked)
        {
            return;
        }
        std::puts("Disconnect slots tracked.");
        locked->clearTrackables();
    };

    // Connect slot and bind tracker.
    signal.connect(slot).bindTrackers(object);

    // Emit the signal.
    signal();

    // Untrack the slot
    auto untrackSlot = [weakObject = comp::weak_ptr<comp::ConnectionTracker>(object)](comp::Connection connection)
    {
        auto locked = weakObject.lock();
        if (!locked)
        {
            return;
        }
        std::puts("Untrack this tracked slot.");
        locked->untrack(connection.get());
    };

    // Connect slot and bind tracker.
    signal.connect(untrackSlot).bindTrackers(object);

    // Emit the signal.
    signal();

    return 0;
}
