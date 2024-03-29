#include "test_base.hpp"
#include <comp/wraps>

class TestObject : public comp::enable_shared_from_this<TestObject>
{
public:
    int voidMethodCallCount = 0;
    int intMethodValue = 0;

    comp::Signal<void(TestObject::*)()> signal{*this};
    comp::Signal<void(TestObject::*)(int)> intSignal{*this};
    comp::Signal<void(TestObject::*)(int&)> intRefSignal{*this};
    comp::Signal<void(TestObject::*)(int, std::string)> intStrSignal{*this};

    void voidMethod()
    {
        ++voidMethodCallCount;
    }
    void intMethod(int value)
    {
        intMethodValue = value;
    }
};

class MemberSignalTest : public SignalTest
{
public:
    explicit MemberSignalTest()
        : object(comp::make_shared<TestObject>())
    {
    }

    comp::shared_ptr<TestObject> object;
};

// The application developer can connect a member signal to a function.
TEST_F(MemberSignalTest, connectMemberSignalToFunction)
{
    auto connection = object->signal.connect(&function);
    EXPECT_TRUE(connection);

    EXPECT_EQ(1, object->signal());
    EXPECT_EQ(1u, functionCallCount);
}

// The application developer can connect a member signal to a function with arguments.
TEST_F(MemberSignalTest, connectToFunctionWithArgument)
{
    auto connection = object->intSignal.connect(&functionWithIntArgument);
    EXPECT_TRUE(connection);

    EXPECT_EQ(1, object->intSignal(10));
    EXPECT_EQ(10, intValue);
}

TEST_F(MemberSignalTest, connectToFunctionWithTwoArguments)
{
    auto object = comp::make_shared<TestObject>();
    auto connection = object->intStrSignal.connect(&functionWithIntAndStringArgument);
    EXPECT_TRUE(connection);

    EXPECT_EQ(1, object->intStrSignal(15, "alpha"));
    EXPECT_EQ(15, intValue);
    EXPECT_EQ("alpha", stringValue);
}

// The application developer can connect a signal to a function with reference arguments.
TEST_F(MemberSignalTest, connectToFunctionWithRefArgument)
{
    auto connection = object->intRefSignal.connect(&functionWithIntRefArgument);
    EXPECT_TRUE(connection);

    int ivalue = 10;
    EXPECT_EQ(1, object->intRefSignal(ivalue));
    EXPECT_EQ(10, intValue);
    EXPECT_EQ(20, ivalue);
}

// The signal should survive the sender object deletion.
TEST_F(MemberSignalTest, deleteSenderObjectFromSlot)
{
    comp::weak_ptr<TestObject> watcher = object;
    auto deleter = [this]()
    {
        object.reset();
    };
    auto checkWatcher = [&watcher]()
    {
        // The object is not destroyed, as the signal activation is holding a reference to the sender.
        EXPECT_FALSE(watcher.expired());
    };
    object->signal.connect(deleter);
    object->signal.connect(checkWatcher);
    EXPECT_EQ(2, object->signal());
    EXPECT_TRUE(watcher.expired());
}

// The application developer should be able to delete a dynamic signal member of the sender object.
TEST_F(MemberSignalTest, deleteSenderSignalInSlot)
{
    using SignalType = comp::Signal<void(TestObject::*)()>;
    auto dynamicSignal = comp::make_unique<SignalType>(*object);

    auto killSignal = [&dynamicSignal]()
    {
        dynamicSignal.reset();
    };
    auto connection1 = dynamicSignal->connect(killSignal);
    auto connection2 = dynamicSignal->connect([](){});
    auto connection3 = dynamicSignal->connect([](){});

    EXPECT_EQ(1, (*dynamicSignal)());
    EXPECT_EQ(nullptr, dynamicSignal);
    EXPECT_FALSE(connection1->isValid());
    EXPECT_FALSE(connection2->isValid());
    EXPECT_FALSE(connection3->isValid());
}

// The application developer should be able to declare a member signal in a PIMPL of a class
// and track the public object.
class Object : public comp::enable_shared_from_this<Object>
{
public:
    using SignalType = comp::Signal<void(Object::*)()>;

    explicit Object()
        : d(new Pimpl(*this))
    {
    }

    ~Object() = default;

    auto& getSignal() const
    {
        return d->signal;
    }

private:
    class Pimpl
    {
    public:
        Object& p_data;
        SignalType signal;
        explicit Pimpl(Object& p)
            : p_data(p)
            , signal(p)
        {
        }
    };

    comp::unique_ptr<Pimpl> d;
};

TEST_F(MemberSignalTest, pimplSignal)
{
    auto object = comp::make_shared<Object>();
    int value = 10;
    auto slot = [&value]()
    {
        value *= 2;
    };
    auto connection = object->getSignal().connect(slot);
    EXPECT_TRUE(connection);
    EXPECT_EQ(1, object->getSignal()());
    EXPECT_EQ(20, value);
}
