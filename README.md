# Component Programming Library - ComP

The library is a header-only c++ library, which contains signals, slots and properties.

## Signals and Slots

The main goal is to provide a design where you can
- connect functions, lambdas, functors and methods to a signal
- ensure that the connected method's receiver object is not destroyed while the slot is activated
- track the lifetime of various objects used in slots, and make the tracked slots disconnect when 
  these objects are destroyed
- do thread-safe signal-slot activation

The connected functions, functors, lambdas or methods are the slots.

The library is unit tested.

The library defines two types of signals, one that takes only a signature, and one that takes a
class beside the signature. You can use both signal types as a standalone signal as well as a class
member signal. The signal type that takes a host class type is initialized with the instance of the
class. The host class must be a shared pointer. When the signal activates, it retains this host
object and makes sure the object is not deleted during signal activation time.

## Properties

To be added later.

## Property Bindings

To be added later.

## Installation, configuration

No special installation is required, you only copy the include folder to some folder you prefer, or
add the include path to reach the library headers in your environment. Then 
- include "comp/signal.hpp" and start using the library.
- if you want to use the library in thread-safe manner, define COMP_CONFIG_THREAD_ENABLED
- if you use the library in shared libraries, you need to export the signal templates, and thus you
  have to define COMP_CONFIG_LIBRARY when building your shared library.
  
## Declaring signals

To declare a signal, use the function signature to define the return type and the argument types.
```cpp
comp::Signal<int(int, std::string_view, int&)> signal;
comp::Signal<void()> fireAndForget;
```

To declare a signal that locks its host object when activated.
```cpp
class Socket : public comp::enable_shared_from_this<Socket>
{
public:
    comp::Signal<void(Socket::*)(int)> opened {*this};
    
    int open()
    {
        // Perform opening
        opened(handler);
        return handler;
    }
    
protected:
    explicit Socket() = default
};
```

## Signal use-cases

### Simple use-cases

```cpp
void function()
{
}

// Declare an argumentum-free signal.
comp::Signal<void()> signal;

// Connect the function to the signal.
signal.connect(function);

// Emit the signal.
signal();
```

When connecting signals to methods, the object of the method must be a shared object:
```cpp
class Object : public comp::enable_shared_from_this<Object>
{
public:
    void method()
    {
    }
};

comp::Signal<void()> signal;

// Connect the method to the signal.
auto object = comp::make_shared<Object>();
signal.connect(object, &Object::method);
```

Connecting signals to functions is illustrated in [this](./examples/connect/example_connect.cpp) example.

### Connect to an other signal
You can connect two signals with exact same signature. You ccan even interconnect them so whenever one 
is activated, the other one is also activated. You cannot re-emit a signal while is activated.

```cpp
comp::Signal<int(std::string&)> signal1;
comp::Signal<int(std::string&)> signal2;

// Interconnect the two signals.
signal1.connect(signal2);
signal2.connect(signal1);
```

### Disconnect a slot

You can disconnect a slot using the connection object either by calling the Signal::disconnect()
method or using the Connection::disconnect() method. 

```cpp
void function()
{
}

// Declare an argumentum-free signal.
comp::Signal<void()> signal;

// Connect the function to the signal.
auto connection = signal.connect(function);

// Emit the signal.
signal();

/// Disconnect the slot.
signal.disconnect(connection);
```

If you want to disconnect a slot within the slot code, use the extended slot signature declaration. 
The extended slot signature is formed using the Connection object followed by the signal's argument 
signature.

```cpp
void function(comp::Connection connection)
{
    // Disconnect the slot when activated.
    connection.disconnect();
}

// Declare an argumentum-free signal.
comp::Signal<void()> signal;

// Connect the function to the signal.
signal.connect(function);

// Emit the signal. When the slot is activated, it disconnects itself.
signal();
```
Disconnecting connections is illustrated in [this](./examples/disconnect/example_disconnect.cpp) example.

### Track the lifetime of a slot

There are use cases where the slots use objects that you want to make sure the slot is not activated
when those are no longer available. The library supports two types of trackers: shared pointers and
pointers of classes derived from TrackerInterface. The later is supported through smart pointers, raw
pointers or intrusive pointers. These all are called trackers.

Trackers are coupled to connections. You can couple a tracker to a connection using the Connection::bind()
method. When the tracker is destroyed, it disconnects the tracked connection. Note that when the tracker
is a shared pointer, the connection is disconnected only during the next activation, or connection
validity check.

Example of using a shared pointer as tracker.
```cpp
class Object : public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

auto object = comp::make_shared<Object>();
comp::Signal<void()> signal;

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
auto connection = signal.connect(slot).bind(object);

// Emit the signal.
signal();

// Reset the object, then see the connection disconnected.
object.reset();
if (!connection)
{
    std::puts("The connection is disconnected.");
}
```

Example of using a pointer to a Tracker-derived object as tracker.
```cpp
class Object : public comp::Tracker
{
public:
    explicit Object() = default;
};

comp::Signal<void()> signal;
auto object = comp::make_unique<Object>();

auto slot = [&object]()
{
    // Use the object.
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
```

### Tracker as type of a shared- or intrusive pointer

When the tracker is a shared pointer with Tracker as type, resetting the pointer of the tracker object 
in the slot may not cause the pointer to release, if the use count of the shared pointer is high enough
to keep the smartt pointer alive. If you want the tracker to "simulate" destruction, i.e you want to 
make sure the tracker disconnects all the tracked objects, call ConnectionTracker::clearTrackables() in
the slot.

```cpp
class Object : public comp::Tracker, public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

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
    locked->clearTrackables();
};

// Connect slot and bind tracker.
signal.connect(slot).bind(object);

// Emit the signal.
signal();
```

If you want to remove a slot from a tracker (untrack) within the activated slot, use the extended
slot signature, and untrack the slot.
```cpp
class Object : public Tracker, public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

auto object = comp::make_shared<Tracker, Object>();
comp::Signal<void()> signal;

// Note: capture shared objects as weak pointer!
auto untrackSlot = [weakObject = comp::weak_ptr<comp::Tracker>(object)](comp::Connection connection)
{
    auto locked = weakObject.lock();
    if (!locked)
    {
        return;
    }
    locked->untrack(connection.get());
};

// Connect slot and bind tracker.
signal.connect(untrackSlot).bind(object);

// Emit the signal.
signal();
```

The same applies to intrusive pointers, see [this](./examples/track_intrusive_tracker/example.cpp) example.

## Licensing
The library is provided as is, under MIT license.
