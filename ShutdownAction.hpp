// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <msiquery.h>

#include <BootstrapperApplicationTypes.h>

namespace Varjo
{

// Returns the shutdown action that the setup leaves to the engine, given the action that WixStdBA
// requested. The setup restarts the machine only with its full UI, where the user chose to restart,
// or when the command line has /forcerestart. A quiet or passive run that needs a restart exits with
// 3010 instead.
constexpr BOOTSTRAPPER_SHUTDOWN_ACTION shutdownAction(BOOTSTRAPPER_SHUTDOWN_ACTION requested, BOOTSTRAPPER_DISPLAY display, bool forceRestart)
{
    if (requested == BOOTSTRAPPER_SHUTDOWN_ACTION_RESTART && display != BOOTSTRAPPER_DISPLAY_FULL && !forceRestart) {
        return BOOTSTRAPPER_SHUTDOWN_ACTION_NONE;
    }
    return requested;
}

}  // namespace Varjo
