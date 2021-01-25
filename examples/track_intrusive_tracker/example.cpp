#include <comp/signal.hpp>
#include <iostream>

class Object : public comp::Tracker, public comp::enable_intrusive_ptr
{
public:
    explicit Object() = default;
};

int main()
{
    comp::Signal<void()> signal;

    auto object = comp::make_shared<comp::Tracker, Object>();

    // Note: capture shared objects as weak pointer!
    auto slot = [locked = object]()
    {
        std::puts("Disconnect slots tracked.");
        locked->disconnectTrackedConnections();
    };

    // Connect slot and bind tracker.
    signal.connect(slot).bind(object);

    // Emit the signal.
    signal();

    // Untrack the slot
    auto untrackSlot = [locked = object](comp::Connection connection)
    {
        std::puts("Untrack this tracked slot.");
        locked->untrack(connection.get());
    };

    // Connect slot and bind tracker.
    signal.connect(untrackSlot).bind(object);

    // Emit the signal.
    signal();

    return 0;
}
