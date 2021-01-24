#include <comp/signal.hpp>
#include <iostream>

class Object : public comp::ConnectionTracker
{
public:
    explicit Object() = default;
};

int main()
{
    comp::Signal<void()> signal;
    auto object = comp::make_unique<Object>();

    auto slot = [&object]()
    {
        // Use the object.
        COMP_UNUSED(object);
    };

    // Connect slot and bind tracker.
    auto connection = signal.connect(slot).bind(object.get());

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
