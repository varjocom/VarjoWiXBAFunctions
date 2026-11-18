// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include "Handles.hpp"

#include <windows.h>

namespace Varjo
{

// Dims a window under a dialog: a black layer at 75 % opacity covers the whole window. The layer
// belongs to the calling thread, lets the mouse through, never takes the focus, and lasts as long as
// the object. A dialog that the owner shows after the layer stays above it. The window class of
// windowClass must be registered.
class Dimmer
{
public:
    Dimmer(HINSTANCE instance, HWND owner);
    ~Dimmer();
    Dimmer(const Dimmer&) = delete;
    Dimmer& operator=(const Dimmer&) = delete;

    static WNDCLASSEXW windowClass(HINSTANCE instance);

private:
    UniqueWindow m_layer;
};

}  // namespace Varjo
