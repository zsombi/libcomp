#include <comp/signal.hpp>
#include <iostream>

class Object : public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

int main()
{
    comp::Signal<void()> signal;

    auto object = comp::make_shared<Object>();

    // Note: capture shared objects as weak pointer!
    auto slot = [weakObject = comp::weak_ptr<Object>(object)]()
    {
        auto locked = weakObject.lock();
        if (!locked)
        {
            return;
        }
        // Use the locked object.
    };

    // Connect slot and bind tracker.
    auto connection = signal.connect(slot).bindTrackers(object);

    // Emit the signal.
    signal();

    // Reset the object, then see the connection disconnected.
    object.reset();
    if (!connection)
    {
        std::puts("The connection is disconnected.");
    }

    return 0;
}
