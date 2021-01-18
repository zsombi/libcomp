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
    auto changed = false;
    auto onBoolPropertyChanged = [&changed](const bool&)
    {
        changed = true;
    };
    property.changed.connect(onBoolPropertyChanged);

    property = false;
    EXPECT_FALSE(changed);
    property = true;
    EXPECT_TRUE(changed);
}

TEST(PropertyTest, changedSignalEmitsOnPropertyCopy)
{
    comp::Property<bool> property;
    comp::Property<bool> property2(true);

    auto changed = false;
    auto onBoolPropertyChanged = [&changed](const bool&)
    {
        changed = true;
    };
    property.changed.connect(onBoolPropertyChanged);

    property = property2;
    EXPECT_TRUE(changed);
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
    void swapOverride(PropertyValue<int>& other) override
    {
        comp::swap(m_data, static_cast<UserData&>(other).m_data);
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
        simple.changed.connect(SignalChangeCount<int>(simpleChangeCount));
        userData.changed.connect(SignalChangeCount<int>(userDataChangeCount));
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
    EXPECT_EQ(comp::PropertyValueStatus::Detached, value->getStatus());

    simple.addPropertyValue(value);
    EXPECT_EQ(comp::PropertyValueStatus::Active, value->getStatus());
    EXPECT_EQ(1u, simpleChangeCount);
}

TEST_F(PropertyValueTest, addSecondUserPropertyValue)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);

    userData.addPropertyValue(value);
    EXPECT_EQ(comp::PropertyValueStatus::Inactive, propertyValue->getStatus());
    EXPECT_EQ(comp::PropertyValueStatus::Active, value->getStatus());
    EXPECT_EQ(1u, userDataChangeCount);
}

TEST_F(PropertyValueTest, removeOriginalValueByWrite)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);
    simple.addPropertyValue(value);

    simple = 10;
    EXPECT_EQ(comp::PropertyValueStatus::Detached, value->getStatus());
}

TEST_F(PropertyValueTest, removeOriginalValueManually)
{
    auto value = comp::make_shared<UserData>(comp::WriteBehavior::Discard);
    userData.addPropertyValue(value);
    userDataChangeCount = 0u;

    userData.removePropertyValue(*propertyValue);
    EXPECT_EQ(0u, userDataChangeCount);
    EXPECT_EQ(comp::PropertyValueStatus::Detached, propertyValue->getStatus());
    EXPECT_EQ(comp::PropertyValueStatus::Active, value->getStatus());
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
        state.changed.connect(SignalChangeCount<int>(stateChangeCount));
    }
};

TEST_F(StatePropertyTest, stateChanged)
{
    EXPECT_EQ(-1, state);

    propertyValue->set(10);
    EXPECT_EQ(10, state);
    EXPECT_EQ(1u, stateChangeCount);
}
