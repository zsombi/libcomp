#include "test_base.hpp"
#include <comp/signal.hpp>

namespace
{

class TestTracker : public comp::Tracker
{
public:
    explicit TestTracker() = default;
};

class IntrusiveTracker : public comp::Tracker, public comp::enable_intrusive_ptr
{
public:
    explicit IntrusiveTracker() = default;
};

class Object : public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

}

class TrackerTest : public SignalTest
{
public:
    explicit TrackerTest() = default;
};

// The application developer should be able to bind trackables to the connection. Any tracker reset disconnects
// the connection.
TEST_F(TrackerTest, connectToTrackable)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto destination = comp::make_unique<TestTracker>();

    auto connection = signal.connect([](){});
    connection.bind(destination.get());
    EXPECT_TRUE(connection);
    destination.reset();
    EXPECT_FALSE(connection);
}

// The application developer should be able to bind shared pointers with the connection. Any tracker reset disconnects
// the connection.
TEST_F(TrackerTest, connectToWeakPointer)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto destination = comp::make_shared<Object>();

    auto connection = signal.connect([](){});
    connection.bind(destination);
    EXPECT_TRUE(connection);
    destination.reset();
    EXPECT_FALSE(connection);
    // emit the signal
    EXPECT_EQ(0, signal().size());
}

// The application developer should be able to bind trackables and shared pointers with the connection. Any tracker reset
// disconnects the connection.
TEST_F(TrackerTest, connectToTrackableAndWeakPointer)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto t1 = comp::make_shared<Object>();
    auto t2 = comp::make_unique<TestTracker>();

    auto connection = signal.connect([](){});
    connection.bind(t1, t2.get());
    EXPECT_TRUE(connection);
    t2.reset();
    EXPECT_FALSE(connection);
}

// The application developer should be able to track multiple slots with the same tracker.
TEST_F(TrackerTest, bindTrackerToMultipleSignals)
{
    using VoidSignalType = comp::Signal<void()>;
    using IntSignalType = comp::Signal<int()>;

    VoidSignalType voidSignal;
    IntSignalType intSignal;
    auto tracker = comp::make_unique<TestTracker>();

    auto connection1 = voidSignal.connect([](){}).bind(tracker.get());
    auto connection2 = intSignal.connect([](){ return 0; }).bind(tracker.get());

    EXPECT_TRUE(connection1);
    EXPECT_TRUE(connection2);
    tracker.reset();
    EXPECT_FALSE(connection1);
    EXPECT_FALSE(connection2);
}

// The application developer should be able to destroy a tracker in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteTrackableInSlotDisconnects)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker = comp::make_unique<TestTracker>();

    auto deleter = [&tracker]()
    {
        tracker.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(tracker.get());
    signal.connect([](){});
    EXPECT_EQ(3, signal().size());
    EXPECT_FALSE(tracker);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal().size());
}

// The application developer should be able to destroy a tracker in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteSharedPtrTrackableInSlotDisconnects)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker = comp::make_shared<Object>();

    auto deleter = [&tracker]()
    {
        tracker.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(tracker);
    signal.connect([](){});
    EXPECT_EQ(3, signal().size());
    EXPECT_FALSE(tracker);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal().size());
}

// The application developer should be able to destroy a tracker in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteOneFromTrackablesInSlotDisconnects_sharedPtr)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker1 = comp::make_shared<Object>();
    auto tracker2 = comp::make_unique<TestTracker>();

    auto deleter = [&tracker1]()
    {
        tracker1.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(tracker1, tracker2.get());
    signal.connect([](){});
    EXPECT_EQ(3, signal().size());
    EXPECT_FALSE(tracker1);
    // the second tracker is not destroyed.
    EXPECT_TRUE(tracker2);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal().size());
}

// The application developer should be able to destroy a tracker in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteOneFromTrackablesInSlotDisconnects_tracker)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker1 = comp::make_shared<Object>();
    auto tracker2 = comp::make_unique<TestTracker>();

    auto deleter = [&tracker2]()
    {
        tracker2.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(tracker1, tracker2.get());
    signal.connect([](){});
    EXPECT_EQ(3, signal().size());
    EXPECT_FALSE(tracker2);
    // the other tracker is not destroyed.
    EXPECT_TRUE(tracker1);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(2, signal().size());
}

// The application developer should be able to disconnect tracked slots in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteOneFromTrackablesInSlotDisconnects_sharedTrackerPtr)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker1 = comp::make_shared<Object>();
    auto tracker2 = comp::make_shared<IntrusiveTracker>();

    auto deleter = [&tracker2]()
    {
        tracker2->disconnectTrackedSlots();
        tracker2.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(tracker1, tracker2);
    signal.connect([](){}).bind(tracker2);
    // The 3rd connection is disconnected by tracker2.
    EXPECT_EQ(2, signal().size());
    EXPECT_FALSE(tracker2);
    // the other tracker is not destroyed.
    EXPECT_TRUE(tracker1);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(1, signal().size());
}

// The application developer should be able to disconnect tracked slots in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteOneFromTrackablesInSlotDisconnects_intrusiveTrackerPtr)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker1 = comp::make_shared<Object>();
    auto tracker2 = comp::make_intrusive<IntrusiveTracker>();

    auto deleter = [&tracker2]()
    {
        tracker2->disconnectTrackedSlots();
        tracker2.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter).bind(tracker1, tracker2);
    signal.connect([](){}).bind(tracker2);
    EXPECT_EQ(2, signal().size());
    EXPECT_FALSE(tracker2);
    // the other tracker is not destroyed.
    EXPECT_TRUE(tracker1);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection);
    EXPECT_EQ(1, signal().size());
}
