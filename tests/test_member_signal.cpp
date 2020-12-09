#include "test_base.hpp"
//#include <sywu/signal.hpp>
#include <sywu/impl/signal_impl.hpp>

class TestObject : public std::enable_shared_from_this<TestObject>
{
public:
    int voidMethodCallCount = 0;
    int intMethodValue = 0;

    sywu::MemberSignal<TestObject> signal{*this};
    sywu::MemberSignal<TestObject, int> intSignal{*this};
    sywu::MemberSignal<TestObject, int&> intRefSignal{*this};
    sywu::MemberSignal<TestObject, int, std::string> intStrSignal{*this};

    void voidMethod()
    {
        ++voidMethodCallCount;
    }
    void intMethod(int value)
    {
        intMethodValue = value;
    }
};

template class sywu::MemberSignal<TestObject>;
template class sywu::MemberSignal<TestObject, int>;
template class sywu::MemberSignal<TestObject, int&>;
template class sywu::MemberSignal<TestObject, int, std::string>;

class MemberSignalTest : public SignalTest
{
public:
    explicit MemberSignalTest()
        : object(std::make_shared<TestObject>())
    {
    }

    std::shared_ptr<TestObject> object;
};

// The application developer can connect a member signal to a function.
TEST_F(MemberSignalTest, connectMemberSignalToFunction)
{
    auto connection = object->signal.connect(&function);
    EXPECT_NE(nullptr, connection);

    EXPECT_EQ(1, object->signal());
    EXPECT_EQ(1u, functionCallCount);
}

// The application developer can connect a member signal to a function with arguments.
TEST_F(MemberSignalTest, connectToFunctionWithArgument)
{
    auto connection = object->intSignal.connect(&functionWithIntArgument);
    EXPECT_NE(nullptr, connection);

    EXPECT_EQ(1, object->intSignal(10));
    EXPECT_EQ(10, intValue);
}

TEST_F(MemberSignalTest, connectToFunctionWithTwoArguments)
{
    auto object = std::make_shared<TestObject>();
    auto connection = object->intStrSignal.connect(&functionWithIntAndStringArgument);
    EXPECT_NE(nullptr, connection);

    EXPECT_EQ(1, object->intStrSignal(15, "alpha"));
    EXPECT_EQ(15, intValue);
    EXPECT_EQ("alpha", stringValue);
}

// The application developer can connect a signal to a function with reference arguments.
TEST_F(MemberSignalTest, connectToFunctionWithRefArgument)
{
    auto connection = object->intRefSignal.connect(&functionWithIntRefArgument);
    EXPECT_NE(nullptr, connection);

    int ivalue = 10;
    EXPECT_EQ(1, object->intRefSignal(ivalue));
    EXPECT_EQ(10, intValue);
    EXPECT_EQ(20, ivalue);
}

TEST_F(MemberSignalTest, deleteSenderObjectFromSlot)
{
    std::weak_ptr<TestObject> watcher = object;
    auto deleter = [this]()
    {
        object.reset();
    };
    auto checkWatcher = [&watcher]()
    {
        EXPECT_FALSE(watcher.expired());
    };
    object->signal.connect(deleter);
    object->signal.connect(checkWatcher);
    EXPECT_EQ(2, object->signal());
    EXPECT_TRUE(watcher.expired());
}
