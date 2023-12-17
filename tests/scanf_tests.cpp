#include "gtest/gtest.h"
#include "dlibc.h"

TEST(Sscanf, Works)
{
    int output;
    int output2;
    int variables_read = d_sscanf("42 19999", "%d %d", &output, &output2);
    EXPECT_EQ(variables_read , 2);
    EXPECT_EQ(output, 42);
    EXPECT_EQ(output2, 19999);
}

TEST(Sscanf, Strings)
{
    #define OUTPUT "HELLOOOOOOOOO"
    char output[80];
    int variables_read = d_sscanf(OUTPUT, "%s", output);
    EXPECT_EQ(strcmp(output, OUTPUT), 0);
    EXPECT_EQ(variables_read, 1);
}
