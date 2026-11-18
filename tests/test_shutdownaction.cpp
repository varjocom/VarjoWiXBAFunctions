// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "ShutdownAction.hpp"

#include <gtest/gtest.h>

using Varjo::shutdownAction;

TEST(ShutdownActionTest, quietRunShouldNotRestart)
{
    EXPECT_EQ(BOOTSTRAPPER_SHUTDOWN_ACTION_NONE, shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART, BOOTSTRAPPER_DISPLAY_NONE, false));
}

TEST(ShutdownActionTest, passiveRunShouldNotRestart)
{
    EXPECT_EQ(BOOTSTRAPPER_SHUTDOWN_ACTION_NONE, shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART, BOOTSTRAPPER_DISPLAY_PASSIVE, false));
}

TEST(ShutdownActionTest, quietRunShouldRestartWithForceRestart)
{
    EXPECT_EQ(BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART, shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART, BOOTSTRAPPER_DISPLAY_NONE, true));
}

TEST(ShutdownActionTest, fullUiShouldRestartWhenRequested)
{
    EXPECT_EQ(BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART, shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART, BOOTSTRAPPER_DISPLAY_FULL, false));
}

TEST(ShutdownActionTest, otherActionsShouldStay)
{
    EXPECT_EQ(BOOTSTRAPPER_SHUTDOWN_ACTION_NONE, shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION_NONE, BOOTSTRAPPER_DISPLAY_NONE, false));
    EXPECT_EQ(
        BOOTSTRAPPER_SHUTDOWN_ACTION_RELOAD_BOOTSTRAPPER, shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION_RELOAD_BOOTSTRAPPER, BOOTSTRAPPER_DISPLAY_NONE, false));
}
