#include "test_base.hpp"
#include <comp/property.hpp>

#include <comp/wrap/type_traits.hpp>


TEST(PropertyTest, declare)
{
    comp::Property<bool> property;
    EXPECT_FALSE(property);
}

TEST(PropertyTest, declarateWithDefaultValue)
{
    comp::Property<bool> property(true);
    EXPECT_TRUE(property);
}

TEST(PropertyTest, copyProperty)
{
    comp::Property<bool> property1(true);
    comp::Property<bool> property2(property1);

    EXPECT_EQ(property1, property2);

    property1 = false;
    property2 = property1;
    EXPECT_EQ(property1, property2);
}

TEST(PropertyTest, changedSignal)
{
    comp::Property<bool> property;
    auto changeCount = size_t(0);
    property.changed.connect(SignalChangeCount(changeCount));

    property = false;
    EXPECT_FALSE(changeCount);
    property = true;
    EXPECT_TRUE(changeCount);
}

TEST(PropertyTest, changedSignalEmitsOnPropertyCopy)
{
    comp::Property<bool> property;
    comp::Property<bool> property2(true);

    auto changeCount = size_t(0);
    property.changed.connect(SignalChangeCount(changeCount));

    property = property2;
    EXPECT_TRUE(changeCount);
}

struct UserData : public comp::PropertyValue<int>
{
    int m_data = -1;
    explicit UserData(comp::WriteBehavior writeBehavior) :
        comp::PropertyValue<int>(writeBehavior)
    {
    }
    DataType evaluateOverride() override
    {
        return m_data;
    }
    bool setOverride(const DataType& data) override
    {
        if (m_data == data)
        {
            return false;
        }
        m_data = data;
        return true;
    }
};

class PropertyValueTest : public ::testing::Test
{
public:
    comp::Property<int> simple;
    comp::shared_ptr<UserData> propertyValue;
    comp::Property<int> userData;

    size_t simpleChangeCount = 0u;
    size_t userDataChangeCount = 0u;

    explicit PropertyValueTest()
        : propertyValue(comp::make_shared<UserData>(comp::WriteBehavior::Keep))
        , userData(propertyValue)
    {
        simple.changed.connect(SignalChangeCount(simpleChangeCount));
        userData.changed.connect(SignalChangeCount(userDataChangeCount));
    }
};

TEST_F(PropertyValueTest, userDataProvider)
{
    EXPECT_EQ(-1, userData);
    userData = 12;
    EXPECT_EQ(12, userData);
    EXPECT_EQ(1u, userDataChangeCount);
}

TEST_F(PropertyValueTest, addPropertyValue)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);

    EXPECT_EQ(0, simple);
    EXPECT_EQ(comp::PropertyValueState::Detached, value->getState());

    simple.addPropertyValue(value);
    EXPECT_EQ(comp::PropertyValueState::Active, value->getState());
    EXPECT_EQ(1u, simpleChangeCount);
}

TEST_F(PropertyValueTest, addSecondUserPropertyValue)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);

    userData.addPropertyValue(value);
    EXPECT_EQ(comp::PropertyValueState::Inactive, propertyValue->getState());
    EXPECT_EQ(comp::PropertyValueState::Active, value->getState());
    EXPECT_EQ(1u, userDataChangeCount);
}

TEST_F(PropertyValueTest, removeOriginalValueByWrite)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);
    simple.addPropertyValue(value);

    simple = 10;
    EXPECT_EQ(comp::PropertyValueState::Detached, value->getState());
}

TEST_F(PropertyValueTest, removeOriginalValueManually)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);
    userData.addPropertyValue(value);
    userDataChangeCount = 0u;

    userData.removePropertyValue(*propertyValue);
    EXPECT_EQ(0u, userDataChangeCount);
    EXPECT_EQ(comp::PropertyValueState::Detached, propertyValue->getState());
    EXPECT_EQ(comp::PropertyValueState::Active, value->getState());
}

class StatePropertyTest : public ::testing::Test
{
public:
    comp::shared_ptr<UserData> propertyValue;
    comp::State<int> state;
    size_t stateChangeCount = 0u;

    explicit StatePropertyTest()
        : propertyValue(comp::make_shared<UserData>(comp::WriteBehavior::Keep))
        , state(propertyValue)
    {
        state.changed.connect(SignalChangeCount(stateChangeCount));
    }
};

TEST_F(StatePropertyTest, stateChanged)
{
    EXPECT_EQ(-1, state);

    propertyValue->set(10);
    EXPECT_EQ(10, state);
    EXPECT_EQ(1u, stateChangeCount);
}

class PropertyBindingTest : public ::testing::Test
{
public:
    comp::Property<int> property;
    comp::Property<int> other;
    comp::Property<float> floatValue{5.f};

    size_t propertyChangeCount = 0u;

    explicit PropertyBindingTest()
    {
        property.changed.connect(SignalChangeCount(propertyChangeCount));
    }
};

TEST_F(PropertyBindingTest, bindExpression)
{
    auto expression = [this]() -> int
    {
        return other;
    };
    property.bind(expression);
    EXPECT_EQ(1u, propertyChangeCount);

    other = 10;
    EXPECT_EQ(2u, propertyChangeCount);
    EXPECT_EQ(10, property);
}

TEST_F(PropertyBindingTest, converterBinding)
{
    auto convert = [this]() -> int
    {
        return int(floatValue);
    };
    property.bind(convert);
    EXPECT_EQ(1u, propertyChangeCount);

    floatValue = 10.f;
    EXPECT_EQ(2u, propertyChangeCount);
    EXPECT_EQ(10, property);
}

TEST_F(PropertyBindingTest, chainedBindExpression)
{
    auto expression = [this]() -> int
    {
        return other;
    };
    property.bind(expression);
    EXPECT_EQ(1u, propertyChangeCount);
    auto convert = [this]() -> int
    {
        return int(floatValue);
    };
    other.bind(convert);
    EXPECT_EQ(2u, propertyChangeCount);

    floatValue = 10.f;
    EXPECT_EQ(3u, propertyChangeCount);
    EXPECT_EQ(10, property);
}

TEST_F(PropertyBindingTest, deletePropertyUsedInBinding)
{
    auto dynamic = comp::make_unique<comp::Property<int>>(11);
    auto expression = [prop = dynamic.get()]() -> int
    {
        return *prop * 10;
    };
    property.bind(expression);
    EXPECT_EQ(1u, propertyChangeCount);
    EXPECT_EQ(110, property);

    dynamic.reset();
    EXPECT_EQ(2u, propertyChangeCount);
    EXPECT_EQ(0, property);
}

TEST_F(PropertyBindingTest, deletePropertyUsedInBindings)
{
    auto dynamic = comp::make_unique<comp::Property<int>>(11);
    auto expression = [prop = dynamic.get()]() -> int
    {
        return *prop * 10;
    };
    property.bind(expression);
    other.bind(expression);
    EXPECT_EQ(1u, propertyChangeCount);
    EXPECT_EQ(110, property);
    EXPECT_EQ(110, other);

    dynamic.reset();
    EXPECT_EQ(2u, propertyChangeCount);
    EXPECT_EQ(0, property);
    EXPECT_EQ(0, other);
}

TEST_F(PropertyBindingTest, expressionWithMultipleProperties)
{
    auto expression = [this]() -> int
    {
        return other + int(floatValue);
    };
    property.bind(expression);
    EXPECT_EQ(1u, propertyChangeCount);

    other = 10;
    EXPECT_EQ(2u, propertyChangeCount);
    EXPECT_EQ(15, property);

    floatValue = 1.f;
    EXPECT_EQ(3u, propertyChangeCount);
}

TEST_F(PropertyBindingTest, debind)
{
    auto expression = [this]() -> int
    {
        return other + int(floatValue);
    };
    property.bind(expression);
    EXPECT_EQ(1u, propertyChangeCount);

    property = 7;
    EXPECT_EQ(2u, propertyChangeCount);

    other = 100;
    EXPECT_EQ(2u, propertyChangeCount);
    EXPECT_EQ(7, property);
}
