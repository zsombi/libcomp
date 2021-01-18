#ifndef TEST_BASE_HPP
#define TEST_BASE_HPP

#include <gtest/gtest.h>
#include <comp/signal.hpp>

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

template <class DerivedClass>
class NotifyDestroyed : public comp::enable_shared_from_this<DerivedClass>
{
public:
    comp::Signal<void()> destroyed;

    explicit NotifyDestroyed() = default;
    ~NotifyDestroyed()
    {
        destroyed();
    }
};

template <typename T>
struct SignalChangeCount
{
    size_t& callCount;
    explicit SignalChangeCount(size_t& counter)
        : callCount(counter)
    {
    }
    void operator()(const T&)
    {
        ++callCount;
    }
};

#endif // TEST_BASE_HPP
