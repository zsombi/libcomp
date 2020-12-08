#include <gtest/gtest.h>
#include <sywu/signal.hpp>
#include <sywu/impl/signal_impl.hpp>

class SignalTest : public ::testing::Test
{
public:
    explicit SignalTest()
    {
        stringValue.clear();
        functionCallCount = 0u;
        intValue = 0u;
    }

    static void function()
    {
        ++functionCallCount;
    }

    static void functionWithIntArgument(int ivalue)
    {
        intValue = ivalue;
    }

    static void functionWithIntRefArgument(int& ivalue)
    {
        intValue = ivalue;
        ivalue *= 2;
    }

    static void functionWithIntAndStringArgument(int ivalue, std::string str)
    {
        intValue = ivalue;
        stringValue = str;
    }

    static inline std::string stringValue;
    static inline size_t functionCallCount = 0u;
    static inline size_t intValue = 0u;
};

class Object1 : public std::enable_shared_from_this<Object1>
{
public:
    void methodWithNoArg()
    {
        ++methodCallCount;
    }

    size_t methodCallCount = 0u;
};

// The application developer can connect a signal to a function.
TEST_F(SignalTest, connectToFunction)
{
    sywu::Signal<> signal;
    auto connection = signal.connect(&function);
    EXPECT_NE(nullptr, connection);

    signal();
    EXPECT_EQ(1u, functionCallCount);
}

// The application developer can connect a signal to a function with arguments.
TEST_F(SignalTest, connectToFunctionWithArgument)
{
    sywu::Signal<int> signal;
    auto connection = signal.connect(&functionWithIntArgument);
    EXPECT_NE(nullptr, connection);

    signal(10);
    EXPECT_EQ(10, intValue);
}
TEST_F(SignalTest, connectToFunctionWithTwoArguments)
{
    sywu::Signal<int, std::string> signal;
    auto connection = signal.connect(&functionWithIntAndStringArgument);
    EXPECT_NE(nullptr, connection);

    signal(15, "alpha");
    EXPECT_EQ(15, intValue);
    EXPECT_EQ("alpha", stringValue);
}

// The application developer can connect a signal to a function with reference arguments.
TEST_F(SignalTest, connectToFunctionWithRefArgument)
{
    sywu::Signal<int&> signal;
    auto connection = signal.connect(&functionWithIntRefArgument);
    EXPECT_NE(nullptr, connection);

    int ivalue = 10;
    signal(std::ref(ivalue));
    EXPECT_EQ(10, intValue);
    EXPECT_EQ(20, ivalue);
}

// The application developer can connect a signal to a method.
TEST_F(SignalTest, connectToMethod)
{
    sywu::Signal<> signal;
    auto object = std::make_shared<Object1>();
    auto connection = signal.connect(object, &Object1::methodWithNoArg);
    EXPECT_NE(nullptr, connection);

    signal();
    EXPECT_EQ(1u, object->methodCallCount);
}

// The application developer can connect a signal to a lambda.
TEST_F(SignalTest, connectToLambda)
{
    sywu::Signal<> signal;
    bool invoked = false;
    auto connection = signal.connect([&invoked]() { invoked = true; });
    EXPECT_NE(nullptr, connection);

    signal();
    EXPECT_TRUE(invoked);
}
\
// The application developer can connect a signal to an other signal.
TEST_F(SignalTest, connectToSignal)
{
    sywu::Signal<> signal1;
    sywu::Signal<> signal2;
    bool invoked = false;

    auto connection = signal1.connect(signal2);
    EXPECT_NE(nullptr, connection);

    signal2.connect([&invoked] () { invoked = true; });
    signal1();
    EXPECT_TRUE(invoked);
}

// When the application developer emits the signal from a slot that is connected to that signal, the signal emit shall not happen
TEST_F(SignalTest, emitSignalThatActivatedTheSlot)
{
    sywu::Signal<> signal;

    auto slot = [&signal]()
    {
        EXPECT_EQ(0u, signal());
    };
    signal.connect(slot);

    EXPECT_EQ(1u, signal());
}

// The application developer can disconnect a connection from a signal through its disconnect function.

// The application developer can disconnect a connection from a signal using the signal dicsonnect function.

// The application developer can connect the same function multiple times.

// The application developer can connect the same method multiple times.

// The application developer can connect the same lambda multiple times.

// The application developer can connect the activated signal to a slot from an activated slot.

// When a signal is blocked, it shall not activate its connections.

// The application developer can block the signal from a slot.

// When the application developer connects the activated signal to a slot from an activated slot,
// that connection is not activated in the same signal activation.

// When a connection is destroyed, it is removed from the signal that owns the connection. The destroyed
// connection is essentially disconnected.

// When the application developer destroys the object of a method that is a slot of a signal connection,
// the connections in which the object is found are invalidated.

// When the signal is destroyed in a slot connected to that signal, it invalidates the connections of the signal.
// The connections following the slot that destroys the signal are not processed.
