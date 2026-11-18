# VarjoBAFunctions

VarjoBAFunctions.dll is the BAFunctions DLL of the setup of the Varjo software. The setup uses
WixStdBA, the standard bootstrapper application of the [WiX Toolset](https://wixtoolset.org/).
WixStdBA loads the DLL and forwards its events to it. The DLL draws what the theme of the setup
cannot express, such as its buttons, its check box and its progress bar. It shows the
errors and warnings of the run on a page of the theme instead of in a message box. It also asks for
the cancel and the restart in dialogs in the style of the theme.

The DLL contains code of the WiX Toolset. It is licensed under the Microsoft Reciprocal License
(MS-RL), and LICENSE.txt has the license text.

## Source

Varjo maintains this code elsewhere and publishes it here for each version of the WiX package of
Varjo that changes it. The message of each commit names the package version, as in "Update to
version 7.0.0-4". The first three numbers of the package version are the version of the WiX
Toolset. Each package version also has a tag, which points to the source of the DLL of that
version. Pull requests are not merged here.

The theme of the setup, with the images and the names of the controls that the DLL uses, is part of
the setup and is not published here.

## Building

CMakeLists.txt builds the DLL and its unit tests with the MSVC toolset and C++17. The DLL links the
static libraries wixstdfn, balutil and dutil of the WiX Toolset, which use the static C runtime. The
build expects these variables:

| Variable | Value |
| --- | --- |
| `WIX_INCLUDE_DIRS` | The header directories of the WiX libraries |
| `WIX_LIBRARIES` | The WiX static libraries, in link order |
| `GTEST_INCLUDE_DIRS` | The header directories of GoogleTest |
| `GTEST_LIBRARIES` | gtest_main and gtest |
| `VERSION` | The version of the DLL, as four numbers separated by dots |

.clang-format holds the formatting of the code.
