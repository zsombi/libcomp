#include <comp/signal>
#include <iostream>

class Object : public comp::DeleteObserver::Notifier
{
public:
    explicit Object() = default;
};

int main()
{
    comp::Signal<void()> signal;

    auto object = comp::make_unique<Object>();

    // Note: capture shared objects as weak pointer!
    auto slot = [&object]()
    {
        if (!object)
        {
            return;
        }
        std::puts("Resetting the object disconnects the watcher connection.");
        object.reset();
    };

    // Connect slot and watch the object deletion.
    signal.connect(slot)->watch(*object);

    // Emit the signal.
    signal();

    object = comp::make_unique<Object>();
    // Unwatch the object
    auto untrackSlot = [&object](comp::ConnectionPtr connection)
    {
        if (!object)
        {
            return;
        }
        std::puts("Unwatch the object.");
        connection->unwatch(*object);
    };

    // Connect slot and bind tracker.
    signal.connect(untrackSlot)->watch(*object);

    // Emit the signal.
    signal();

    return 0;
}
