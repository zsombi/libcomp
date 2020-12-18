#include "test_base.hpp"
#include <sywu/impl/signal_impl.hpp>

class T1 : public sywu::Trackable
{
public:
    explicit T1() = default;
};

class Object : public std::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};


class TrackableTest : public SignalTest
{
public:
    explicit TrackableTest() = default;
};

// The application developer should be able to bind trackables to the connection. Any trackable reset disconnects
// the connection.
TEST_F(TrackableTest, connectToTrackable)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto destination = std::make_unique<T1>();

    auto connection = signal.connect([](){});
    connection.bind(destination.get());
    EXPECT_TRUE(connection);
    destination.reset();
    EXPECT_FALSE(connection);
}

// The application developer should be able to bind shared pointers with the connection. Any trackable reset disconnects
// the connection.
TEST_F(TrackableTest, connectToWeakPointer)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto destination = std::make_shared<Object>();

    auto connection = signal.connect([](){});
    connection.bind(destination);
    EXPECT_TRUE(connection);
    destination.reset();
    EXPECT_FALSE(connection);
    // emit the signal
    EXPECT_EQ(0, signal());
}

// The application developer should be able to bind trackables and shared pointers with the connection. Any trackable reset
// disconnects the connection.
TEST_F(TrackableTest, connectToTrackableAndWeakPointer)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto t1 = std::make_shared<Object>();
    auto t2 = std::make_unique<T1>();

    auto connection = signal.connect([](){});
    connection.bind(t1, t2.get());
    EXPECT_TRUE(connection);
    t2.reset();
    EXPECT_FALSE(connection);
}

// The application developer should be able to track multiple slots with the same trackable.
TEST_F(TrackableTest, bindTrackerToMultipleSignals)
{
    using VoidSignalType = sywu::Signal<void()>;
    using IntSignalType = sywu::Signal<int()>;

    VoidSignalType voidSignal;
    IntSignalType intSignal;
    auto trackable = std::make_unique<T1>();

    auto connection1 = voidSignal.connect([](){}).bind(trackable.get());
    auto connection2 = intSignal.connect([](){ return 0; }).bind(trackable.get());

    EXPECT_TRUE(connection1);
    EXPECT_TRUE(connection2);
    trackable.reset();
    EXPECT_FALSE(connection1);
    EXPECT_FALSE(connection2);
}

// The application developer should be able to destroy a trackable in the slot to shich the trackable is bount to.
TEST_F(TrackableTest, deleteTrackableInSlotDisconnects)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto trackable = std::make_unique<T1>();

    auto deleter = [&trackable]()
    {
        trackable.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(trackable.get());
    signal.connect([](){});
    EXPECT_EQ(3, signal());
    EXPECT_FALSE(trackable);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal());
}

// The application developer should be able to destroy a trackable in the slot to shich the trackable is bount to.
TEST_F(TrackableTest, deleteSharedPtrTrackableInSlotDisconnects)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto trackable = std::make_shared<Object>();

    auto deleter = [&trackable]()
    {
        trackable.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(trackable);
    signal.connect([](){});
    EXPECT_EQ(3, signal());
    EXPECT_FALSE(trackable);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal());
}

// The application developer should be able to destroy a trackable in the slot to shich the trackable is bount to.
TEST_F(TrackableTest, deleteOneTrackableInSlotDisconnects)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto trackable1 = std::make_shared<Object>();
    auto trackable2 = std::make_unique<T1>();

    auto deleter = [&trackable1]()
    {
        trackable1.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(trackable1, trackable2.get());
    signal.connect([](){});
    EXPECT_EQ(3, signal());
    EXPECT_FALSE(trackable1);
    // the second trackable is not destroyed.
    EXPECT_TRUE(trackable2);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal());
}
