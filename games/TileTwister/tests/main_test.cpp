#include <gtest/gtest.h>

TEST(SanityCheck, BasicAssertions) {
  EXPECT_TRUE(true);
  EXPECT_EQ(1 + 1, 2);
}
