#include <comp/signal.hpp>
#include <iostream>

class Object : public comp::Tracker, public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

int main()
{
    comp::Signal<void()> signal;

    auto object = comp::make_shared<comp::Tracker, Object>();

    // Note: capture shared objects as weak pointer!
    auto slot = [weakObject = comp::weak_ptr<comp::Tracker>(object)]()
    {
        auto locked = weakObject.lock();
        if (!locked)
        {
            return;
        }
        std::puts("Disconnect slots tracked.");
        locked->disconnectTrackedSlots();
    };

    // Connect slot and bind tracker.
    signal.connect(slot).bind(object);

    // Emit the signal.
    signal();

    // Untrack the slot
    auto untrackSlot = [weakObject = comp::weak_ptr<comp::Tracker>(object)](comp::Connection connection)
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
    signal.connect(untrackSlot).bind(object);

    // Emit the signal.
    signal();

    return 0;
}
