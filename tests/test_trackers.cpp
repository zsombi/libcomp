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
    EXPECT_TRUE(connection.isValid());
    destination.reset();
    EXPECT_FALSE(connection.isValid());
}

// The application developer should be able to bind shared pointers with the connection. Any trackable reset disconnects
// the connection.
TEST_F(TrackableTest, connectToWeakPointer)
{
    using SignalType = sywu::Signal<void()>;
    SignalType signal;
    auto destination = std::make_shared<Object>();

    bool v = std::is_base_of_v<std::shared_ptr<Object>, decltype(destination)>;
    EXPECT_TRUE(v);

    auto connection = signal.connect([](){});
    connection.bind(destination);
    EXPECT_TRUE(connection.isValid());
    destination.reset();
    EXPECT_FALSE(connection.isValid());
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
    EXPECT_TRUE(connection.isValid());
    t2.reset();
    EXPECT_FALSE(connection.isValid());
}
