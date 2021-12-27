# Component Programming Library - ComP

The library is a header-only c++ library, which contains signals, connected slots and properties.

## Signals, Connections

The main goal is to provide a design where you can
- connect functions, lambdas, functors and methods to a signal; these are called slots
- bind connection validity to objects, which notify the connection when those are destroyed
- ensure that the connected method's receiver object is kept alive while the slot is activated
- do thread-safe signal-slot activation

The library is unit tested.

The library defines two types of signals: 
- one that you can use in any context
- one that you can use as class members.

Both signal types take a signature, which defines the arguments and the return type of the signal.
When a class member signal activates, it retains its host object and ensures the object is not 
deleted during signal activation time.

## Installation, configuration

No special installation is required, you only copy the include folder to some folder you prefer, or
add the include path to reach the library headers in your environment. Then 
- include "comp/signal" and start using the library.
- if you want to use the library in thread-safe manner, define COMP_CONFIG_THREAD_ENABLED
- if you use the library in shared libraries, you need to export the signal templates, and thus you
  have to define COMP_CONFIG_LIBRARY when building your shared library.
  
## Declaring signals

To declare a signal, use the function signature to define the return type and the argument types.
```cpp
comp::Signal<int(int, std::string_view, int&)> signal;
comp::Signal<void()> fireAndForget;
```

To declare a member signal, which locks its host object when activated.
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
You can connect two signals with exact same signature. You can even interconnect them so whenever one 
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
method or using the SignalConcept::Connection::disconnect() method. 

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
void function(comp::SignalConcept::ConnectionPtr connection)
{
    // Disconnect the slot when activated.
    connection->disconnect();
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

There are cases when a slot depends on the lifetime of an object. A connection to a slot can track
the lifetime of such objects. The object must derive from comp::DeleteObserver::Notifier. You can 
track the deletion of such object by couplig the object to the connection calling the 
comp::SignalConcept::Connection::watch() method. To decouple an object, call the 
comp::SignalConcept::Connection::unwatch() method.

Example of using a shared pointer as tracker.
```cpp
class Object : public comp::DeleteObserver::Notifier, public comp::enable_shared_from_this<Object>
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

// Connect slot and watch the object it uses.
auto connection = signal.connect(slot);
connection->watch(object);

// Emit the signal.
signal();

// Reset the object, then see the connection disconnected.
object.reset();
if (!connection->isValid())
{
    std::puts("The connection is disconnected.");
}
```

## Licensing
The library is provided as is, under MIT license.
