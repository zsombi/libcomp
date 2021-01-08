#include "test_base.hpp"
#include <sywu/impl/signal_impl.hpp>

template class sywu::Signal<void()>;
template class sywu::Signal<void(int)>;
template class sywu::Signal<void(int&)>;
template class sywu::Signal<void(int, std::string)>;

class Object1 : public sywu::enable_shared_from_this<Object1>
{
public:
    void methodWithNoArg()
    {
        ++methodCallCount;
    }

    void autoDisconnect(sywu::Connection connection)
    {
        connection.disconnect();
    }

    size_t methodCallCount = 0u;
};

// The application developer can connect a signal to a function.
TEST_F(SignalTest, connectToFunction)
{
    sywu::Signal<void()> signal;
    auto connection = signal.connect(&function);
    EXPECT_TRUE(connection);

    EXPECT_EQ(1, signal());
    EXPECT_EQ(1u, functionCallCount);
}

// The application developer can connect a signal to a function with arguments.
TEST_F(SignalTest, connectToFunctionWithArgument)
{
    sywu::Signal<void(int)> signal;
    auto connection = signal.connect(&functionWithIntArgument);
    EXPECT_TRUE(connection);

    signal(10);
    EXPECT_EQ(10, intValue);
}
TEST_F(SignalTest, connectToFunctionWithTwoArguments)
{
    sywu::Signal<void(int, std::string)> signal;
    auto connection = signal.connect(&functionWithIntAndStringArgument);
    EXPECT_TRUE(connection);

    signal(15, "alpha");
    EXPECT_EQ(15, intValue);
    EXPECT_EQ("alpha", stringValue);
}

// The application developer can connect a signal to a function with reference arguments.
TEST_F(SignalTest, connectToFunctionWithRefArgument)
{
    sywu::Signal<void(int&)> signal;
    auto connection = signal.connect(&functionWithIntRefArgument);
    EXPECT_TRUE(connection);

    int ivalue = 10;
    signal(sywu::ref(ivalue));
    EXPECT_EQ(10, intValue);
    EXPECT_EQ(20, ivalue);
}

// The application developer can connect a signal to a method.
TEST_F(SignalTest, connectToMethod)
{
    sywu::Signal<void()> signal;
    auto object = sywu::make_shared<Object1>();
    auto connection = signal.connect(object, &Object1::methodWithNoArg);
    EXPECT_TRUE(connection);

    signal();
    EXPECT_EQ(1u, object->methodCallCount);
}

// The application developer can connect a signal to a lambda.
TEST_F(SignalTest, connectToLambda)
{
    sywu::Signal<void()> signal;
    bool invoked = false;
    auto connection = signal.connect([&invoked]() { invoked = true; });
    EXPECT_TRUE(connection);

    signal();
    EXPECT_TRUE(invoked);
}
\
// The application developer can connect a signal to an other signal.
TEST_F(SignalTest, connectToSignal)
{
    sywu::Signal<void()> signal1;
    sywu::Signal<void()> signal2;
    bool invoked = false;

    auto connection = signal1.connect(signal2);
    EXPECT_TRUE(connection);

    signal2.connect([&invoked] () { invoked = true; });
    signal1();
    EXPECT_TRUE(invoked);
}

// When the application developer emits the signal from a slot that is connected to that signal, the signal emit shall not happen
TEST_F(SignalTest, emitSignalThatActivatedTheSlot)
{
    sywu::Signal<void()> signal;

    auto slot = [&signal]()
    {
        EXPECT_EQ(0u, signal());
    };
    signal.connect(slot);

    EXPECT_EQ(1u, signal());
}

// It should be possible for an application developer to receive the connection that is associated to a slot as the first argument.
TEST_F(SignalTest, slotWithConnection)
{
    using VoidSignalType = sywu::Signal<void()>;
    using IntSignalType = sywu::Signal<void(int)>;

    auto onVoidSignal = [](sywu::Connection connection)
    {
        connection.disconnect();
    };
    auto onIntSignal = [](sywu::Connection connection, int)
    {
        connection.disconnect();
    };
    VoidSignalType voidSignal;
    IntSignalType intSignal;
    auto voidConnection = voidSignal.connect(onVoidSignal);
    EXPECT_TRUE(voidConnection);
    auto intConnection = intSignal.connect(onIntSignal);
    EXPECT_TRUE(intConnection);
    voidSignal();
    EXPECT_FALSE(voidConnection);
    intSignal(10);
    EXPECT_FALSE(intConnection);
}

// It should be possible for an application developer to receive the connection that is associated to a slot as the first argument.
TEST_F(SignalTest, methodSlotWithConnection)
{
    using VoidSignalType = sywu::Signal<void()>;

    VoidSignalType voidSignal;
    auto object = sywu::make_shared<Object1>();

    auto voidConnection = voidSignal.connect(object, &Object1::autoDisconnect);
    EXPECT_TRUE(voidConnection);
    voidSignal();
    EXPECT_FALSE(voidConnection);
}

// The application developer can disconnect a connection from a signal using the signal disconnect function.
TEST_F(SignalTest, disconnectWithSignal)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto slot = [](sywu::Connection connection)
    {
        connection.disconnect();
    };
    auto connection = signal.connect(slot);
    EXPECT_TRUE(connection);
    EXPECT_EQ(1u, signal());
    EXPECT_FALSE(connection);
    EXPECT_EQ(0u, signal());
}

// The application developer can connect the same function multiple times.
TEST_F(SignalTest, connectFunctionManyTimes)
{
    sywu::Signal<void()> signal;
    signal.connect(&function);
    signal.connect(&function);
    signal.connect(&function);

    EXPECT_EQ(functionCallCount, signal());
}

// The application developer can connect the same method multiple times.
TEST_F(SignalTest, connectMethodManyTimes)
{
    sywu::Signal<void()> signal;
    auto object = sywu::make_shared<Object1>();
    signal.connect(object, &Object1::methodWithNoArg);
    signal.connect(object, &Object1::methodWithNoArg);
    signal.connect(object, &Object1::methodWithNoArg);

    EXPECT_EQ(object->methodCallCount, signal());
}

// The application developer can connect the same lambda multiple times.
TEST_F(SignalTest, connectLambdaManyTimes)
{
    sywu::Signal<void()> signal;
    int invokeCount = 0;
    auto slot = [&invokeCount] { ++invokeCount; };
    signal.connect(slot);
    signal.connect(slot);
    signal.connect(slot);

    EXPECT_EQ(invokeCount, signal());
}

// The application developer can connect the activated signal to a slot from an activated slot.
TEST_F(SignalTest, connectToTheInvokingSignal)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;

    auto slot = [&signal]()
    {
        signal.connect(&function);
    };
    signal.connect(slot);
    EXPECT_EQ(1, signal());
    EXPECT_EQ(2, signal());
    EXPECT_EQ(3, signal());
}

// When a signal is blocked, it shall not activate its connections.
TEST_F(SignalTest, blockSignal)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    signal.connect([](){});
    signal.connect([](){});
    signal.connect([](){});

    signal.setBlocked(true);
    EXPECT_EQ(0, signal());
    signal.setBlocked(false);
    EXPECT_EQ(3, signal());
}

// The application developer can block the signal from a slot. Blocking the signal from the slot does not affect the
// active emit loop of the signal.
TEST_F(SignalTest, blockSignalFromSlot)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    signal.connect([](){});
    signal.connect([&signal](){ signal.setBlocked(true); });
    signal.connect([](){});

    EXPECT_EQ(3, signal());
    EXPECT_EQ(0, signal());
}

// When the application developer connects the activated signal to a slot from an activated slot,
// that connection is not activated in the same signal activation.
TEST_F(SignalTest, connectionFromSlotGetsActivatedNextTime)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;

    auto slot = [&signal]()
    {
        signal.connect(&function);
    };
    signal.connect(slot);
    EXPECT_EQ(1, signal());
    EXPECT_EQ(0, functionCallCount);

    EXPECT_EQ(2, signal());
    EXPECT_EQ(1, functionCallCount);
}

// When the application developer destroys the object of a method that is a slot of a signal connection,
// the connections in which the object is found are invalidated.
TEST_F(SignalTest, signalsConnectedToAnObjectThatGetsDeleted)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal1;
    SignalType signal2;
    SignalType signal3;

    auto object = sywu::make_shared<Object1>();
    auto weakObject = sywu::weak_ptr<Object1>(object);
    auto connection1 = signal1.connect(object, &Object1::methodWithNoArg);
    auto connection2 = signal2.connect(object, &Object1::methodWithNoArg);
    auto connection3 = signal3.connect(object, &Object1::methodWithNoArg);
    auto objectDeleter = [&object]()
    {
        object.reset();
    };
    signal1.connect(objectDeleter);
    EXPECT_EQ(1, weakObject.use_count());
    signal1();
    EXPECT_EQ(0, weakObject.use_count());
    EXPECT_FALSE(connection1);
    EXPECT_FALSE(connection2);
    EXPECT_FALSE(connection3);
}
TEST_F(SignalTest, signalsConnectedToAnObjectThatGetsDeleted_noConenctionHolding)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal1;
    SignalType signal2;
    SignalType signal3;

    auto object = sywu::make_shared<Object1>();
    signal1.connect(object, &Object1::methodWithNoArg);
    signal2.connect(object, &Object1::methodWithNoArg);
    signal3.connect(object, &Object1::methodWithNoArg);
    auto objectDeleter = [&object]()
    {
        object.reset();
    };
    signal1.connect(objectDeleter);
    EXPECT_EQ(2, signal1());
    EXPECT_EQ(0, signal2());
    EXPECT_EQ(0, signal3());
}
TEST_F(SignalTest, receiverObjectDeleted)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto object = sywu::make_shared<Object1>();
    signal.connect(object, &Object1::methodWithNoArg);

    EXPECT_EQ(1, signal());
    object.reset();
    EXPECT_EQ(0, signal());
}

// When the signal is destroyed in a slot connected to that signal, it invalidates the connections of the signal.
// The connections following the slot that destroys the signal are not processed.
TEST_F(SignalTest, deleteEmitterSignalFromSlot)
{
    using SignalType = sywu::Signal<void()>;
    auto signal = sywu::make_unique<SignalType>();

    auto killSignal = [&signal]()
    {
        signal.reset();
    };
    auto connection1 = signal->connect(killSignal);
    auto connection2 = signal->connect([](){});
    auto connection3 = signal->connect([](){});

    EXPECT_EQ(1, (*signal)());
    EXPECT_EQ(nullptr, signal);
    EXPECT_FALSE(connection1);
    EXPECT_FALSE(connection2);
    EXPECT_FALSE(connection3);
}

TEST_F(SignalTest, deleteConnectedSignal)
{
    using SenderType = sywu::Signal<void()>;
    using ReceiverType = sywu::Signal<void()>;
    auto sender = sywu::make_unique<SenderType>();
    auto receiver = sywu::make_unique<ReceiverType>();

    auto connection = sender->connect(*receiver);
    EXPECT_TRUE(connection);
    EXPECT_EQ(1, (*sender)());
    receiver.reset();
    EXPECT_FALSE(connection);
    EXPECT_EQ(0, (*sender)());
}

struct Functor
{
    void operator()(int& value)
    {
        value *= 10;
    }
};

TEST_F(SignalTest, connectToFunctor)
{
    using SignalType = sywu::Signal<void(int&)>;
    SignalType signal;
    Functor fn;
    auto connection = signal.connect(fn);
    EXPECT_TRUE(connection);

    int value = 10;
    EXPECT_EQ(1, signal(value));
    EXPECT_EQ(100, value);
}

namespace
{

class Base;
using BasePtr = sywu::shared_ptr<Base>;
using BaseWeakPtr = sywu::weak_ptr<Base>;

class Base : public NotifyDestroyed<Base>
{
public:
    BaseWeakPtr m_pair;
    explicit Base() = default;

    void setPair(BasePtr pair)
    {
        m_pair = pair;
        pair->destroyed.connect(shared_from_this(), &Base::onPairDestroyed);
    }

    void onPairDestroyed()
    {
        m_pair.reset();
    }
};

class Client : public Base
{
public:
    explicit Client() = default;
};

class Server : public Base
{
public:
    sywu::MemberSignal<Server, void()> closed{*this};
    explicit Server() = default;
};

}

TEST_F(SignalTest, pairNotifyDestruction)
{
    auto server = sywu::make_shared<Base, Server>();
    auto client = sywu::make_shared<Base, Client>();
    EXPECT_EQ(1, server.use_count());
    EXPECT_EQ(0, server->m_pair.use_count());

    server->setPair(client);
    EXPECT_EQ(1, server.use_count());
    EXPECT_EQ(1, server->m_pair.use_count());

    client->setPair(server);
    EXPECT_EQ(1, server.use_count());
    EXPECT_EQ(1, server->m_pair.use_count());

    client.reset();
    EXPECT_EQ(1, server.use_count());
    EXPECT_EQ(0, server->m_pair.use_count());
}

class ContextEmit : public SignalTest
{
public:
    sywu::Signal<int()> intSignal;

    explicit ContextEmit()
    {
        auto onOne = []() -> int
        {
            return 1;
        };
        auto onTen = []() -> int
        {
            return 10;
        };

        intSignal.connect(onOne);
        intSignal.connect(onTen);
    }

    class Accumulate : public sywu::Collector<Accumulate>
    {
    public:
        void handleResult(sywu::Connection, int result)
        {
            results.push_back(result);
        }
        sywu::vector<int> results;
    };
    class Summ : public sywu::Collector<Summ>
    {
    public:
        void handleResult(sywu::Connection, int result)
        {
            grandTotal += result;
        }
        int grandTotal = 0;
    };
};

// The application developer should be able to use emit contexts to accumulate the results of the slots connected to a signal
// that has a return value.
TEST_F(ContextEmit, accumulateResults)
{
    auto context = intSignal.operator()<Accumulate>();
    EXPECT_EQ(2, context.results.size());
    EXPECT_EQ(1, context.results[0]);
    EXPECT_EQ(10, context.results[1]);
}

// The application developer should be able to use emit contexts to summ the results of the slots connected to a signal
// that has a return value.
TEST_F(ContextEmit, summResults)
{
    auto context = intSignal.operator()<Summ>();
    EXPECT_EQ(11, context.grandTotal);
}
