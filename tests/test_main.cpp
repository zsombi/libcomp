#include <gtest/gtest.h>

#include <comp/wraps>

namespace
{

template <class T>
struct Deleter
{
    void operator()(T*)
    {
    }
};

struct Data
{
};

using UniquePtr = comp::unique_ptr<Data>;
using UniqueDeleterPtr = comp::unique_ptr<Data, Deleter<Data>>;
using SharedPtr = comp::shared_ptr<Data>;
using WeakPtr = comp::weak_ptr<Data>;

}

TEST(MemoryTraits, detectUniquePtr)
{
    EXPECT_TRUE(comp::is_unique_ptr_v<UniquePtr>);
    EXPECT_TRUE(comp::is_unique_ptr_v<UniqueDeleterPtr>);
    EXPECT_FALSE(comp::is_unique_ptr_v<SharedPtr>);
    EXPECT_FALSE(comp::is_unique_ptr_v<WeakPtr>);
}

TEST(MemoryTraits, detectSharedPtr)
{
    EXPECT_FALSE(comp::is_shared_ptr_v<UniquePtr>);
    EXPECT_FALSE(comp::is_shared_ptr_v<UniqueDeleterPtr>);
    EXPECT_TRUE(comp::is_shared_ptr_v<SharedPtr>);
    EXPECT_FALSE(comp::is_shared_ptr_v<WeakPtr>);
}

TEST(MemoryTraits, detectWeakPtr)
{
    EXPECT_FALSE(comp::is_weak_ptr_v<UniquePtr>);
    EXPECT_FALSE(comp::is_weak_ptr_v<UniqueDeleterPtr>);
    EXPECT_FALSE(comp::is_weak_ptr_v<SharedPtr>);
    EXPECT_TRUE(comp::is_weak_ptr_v<WeakPtr>);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
