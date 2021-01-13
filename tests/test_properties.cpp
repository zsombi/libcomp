#include "test_base.hpp"
#include <comp/concept/property.hpp>


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
