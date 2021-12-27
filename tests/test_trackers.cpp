#include "test_base.hpp"
#include <comp/wraps>
#include <comp/utilities>

namespace
{

class TestNotifier : public comp::DeleteObserver::Notifier
{
public:
    explicit TestNotifier() = default;
};

class Object : public comp::enable_shared_from_this<Object>
{
public:
    explicit Object() = default;
};

using TrackerTest = SignalTest;

}

// The application developer should be able to bind trackables to the connection. Any tracker reset disconnects
// the connection.
TEST_F(TrackerTest, connectToTrackable)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto destination = comp::make_unique<TestNotifier>();

    auto connection = signal.connect([](){});
    connection->watch(*destination);
    EXPECT_TRUE(connection->isValid());
    destination.reset();
    EXPECT_FALSE(connection->isValid());
}

// The application developer should be able to track multiple slots with the same tracker.
TEST_F(TrackerTest, bindTrackerToMultipleSignals)
{
    using VoidSignalType = comp::Signal<void()>;
    using IntSignalType = comp::Signal<int()>;

    VoidSignalType voidSignal;
    IntSignalType intSignal;
    auto tracker = comp::make_unique<TestNotifier>();

    auto connection1 = voidSignal.connect([](){});
    auto connection2 = intSignal.connect([](){ return 0; });
    connection1->watch(*tracker);
    connection2->watch(*tracker);

    EXPECT_TRUE(connection1->isValid());
    EXPECT_TRUE(connection2->isValid());
    tracker.reset();
    EXPECT_FALSE(connection1->isValid());
    EXPECT_FALSE(connection2->isValid());
}

// The application developer should be able to destroy a tracker in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteTrackableInSlotDisconnects)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker = comp::make_unique<TestNotifier>();

    auto deleter = [&tracker]()
    {
        tracker.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter);
    connection->watch(*tracker);
    signal.connect([](){});
    EXPECT_EQ(3, signal());
    EXPECT_FALSE(tracker);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection->isValid());
    EXPECT_EQ(2, signal());
}

// The application developer should be able to destroy a tracker in the slot to shich the tracker is bount to.
TEST_F(TrackerTest, deleteOneFromTrackablesInSlotDisconnects_tracker)
{
    using SignalType = comp::Signal<void()>;
    SignalType signal;
    auto tracker1 = comp::make_unique<TestNotifier>();
    auto tracker2 = comp::make_unique<TestNotifier>();

    auto deleter = [&tracker2]()
    {
        tracker2.reset();
    };

    // connect 3 slots
    signal.connect([](){});
    auto connection = signal.connect(deleter);
    connection->watch(*tracker1);
    connection->watch(*tracker2);
    signal.connect([](){});
    EXPECT_EQ(3, signal());
    EXPECT_FALSE(tracker2);
    // the other tracker is not destroyed.
    EXPECT_TRUE(tracker1);
    // The deleter slot is disconnected.
    EXPECT_FALSE(connection->isValid());
    EXPECT_EQ(2, signal());
}
