#include "gtest/gtest.h"
#include "printf.h"
#include "dlibc.h"

TEST(Printf, Works)
{
    char BUFFER[32];
    d_snprintf(BUFFER, sizeof(BUFFER), "Hello %d world!\n", 123);
    EXPECT_EQ(strcmp(BUFFER, "Hello 123 world!\n"), 0);
}

TEST(Printf, CharLimit)
{
    char BUFFER[4];
    int result = d_snprintf(BUFFER, sizeof(BUFFER), "Hell");
    EXPECT_EQ(strcmp(BUFFER, "Hel"), 0);
    EXPECT_EQ(result, 3);
}
