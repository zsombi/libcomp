#include <gtest/gtest.h>

#include <sywu/wrap/memory.hpp>

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

using UniquePtr = sywu::unique_ptr<Data>;
using UniqueDeleterPtr = sywu::unique_ptr<Data, Deleter<Data>>;
using SharedPtr = sywu::shared_ptr<Data>;
using WeakPtr = sywu::weak_ptr<Data>;

}

TEST(MemoryTraits, detectUniquePtr)
{
    EXPECT_TRUE(sywu::is_unique_ptr_v<UniquePtr>);
    EXPECT_TRUE(sywu::is_unique_ptr_v<UniqueDeleterPtr>);
    EXPECT_FALSE(sywu::is_unique_ptr_v<SharedPtr>);
    EXPECT_FALSE(sywu::is_unique_ptr_v<WeakPtr>);
}

TEST(MemoryTraits, detectSharedPtr)
{
    EXPECT_FALSE(sywu::is_shared_ptr_v<UniquePtr>);
    EXPECT_FALSE(sywu::is_shared_ptr_v<UniqueDeleterPtr>);
    EXPECT_TRUE(sywu::is_shared_ptr_v<SharedPtr>);
    EXPECT_FALSE(sywu::is_shared_ptr_v<WeakPtr>);
}

TEST(MemoryTraits, detectWeakPtr)
{
    EXPECT_FALSE(sywu::is_weak_ptr_v<UniquePtr>);
    EXPECT_FALSE(sywu::is_weak_ptr_v<UniqueDeleterPtr>);
    EXPECT_FALSE(sywu::is_weak_ptr_v<SharedPtr>);
    EXPECT_TRUE(sywu::is_weak_ptr_v<WeakPtr>);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
