// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text. BAFunctionsCreate and BAFunctionsDestroy follow the BAFunctions
// sample of the WiX Toolset, copyright (c) .NET Foundation and contributors.

// The entry points of the DLL. WixStdBA calls BAFunctionsCreate after it has loaded the DLL, and
// BAFunctionsDestroy on the thread of the engine events before it unloads the DLL. By then the UI
// thread has ended and the main window is gone.

#include "ChoiceDialog.hpp"
#include "Dimmer.hpp"
#include "Images.hpp"
#include "VarjoBAFunctions.hpp"
#include "WindowUtil.hpp"

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4458)  // GDI+ headers hide class members.
#endif
#include <gdiplus.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <BalBaseBAFunctionsProc.h>
#include <balutil.h>

#include <memory>
#include <new>

namespace Varjo
{
namespace
{

// The logging and the engine functions of balutil, which BalInitialize starts.
class BalSession
{
public:
    explicit BalSession(IBootstrapperEngine* engine);
    ~BalSession();
    BalSession(const BalSession&) = delete;
    BalSession& operator=(const BalSession&) = delete;
};

class GdiplusSession
{
public:
    GdiplusSession();
    ~GdiplusSession();
    GdiplusSession(const GdiplusSession&) = delete;
    GdiplusSession& operator=(const GdiplusSession&) = delete;

    bool isRunning() const;

private:
    ULONG_PTR m_token = 0;
    bool m_running = false;
};

struct BAFunctionsReleaser {
    void operator()(VarjoBAFunctions* functions) const { functions->Release(); }
};

// Everything that the DLL holds between BAFunctionsCreate and BAFunctionsDestroy. The members go in
// the reverse order: the BAFunctions object before the images, and the images before GDI+.
class Module
{
public:
    Module(HINSTANCE instance, IBootstrapperEngine* engine);
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    HRESULT createBAFunctions(const BA_FUNCTIONS_CREATE_ARGS& args, BA_FUNCTIONS_CREATE_RESULTS& results);

private:
    const HINSTANCE m_instance;
    BalSession m_bal;
    GdiplusSession m_gdiplus;
    WindowClass m_dimmerClass;
    WindowClass m_choiceDialogClass;
    ImageCache m_images;
    std::unique_ptr<VarjoBAFunctions, BAFunctionsReleaser> m_functions;
};

HRESULT WINAPI varjoBAFunctionsProc(BA_FUNCTIONS_MESSAGE message, const LPVOID args, LPVOID results, LPVOID context);

HINSTANCE g_instance = nullptr;
// The module lives from BAFunctionsCreate to BAFunctionsDestroy. The pointer is raw, because a
// global with a destructor would run it in DLL_PROCESS_DETACH, under the loader lock.
Module* g_module = nullptr;

}  // namespace
}  // namespace Varjo

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        Varjo::g_instance = instance;
    }
    return TRUE;
}

extern "C" HRESULT WINAPI BAFunctionsCreate(const BA_FUNCTIONS_CREATE_ARGS* args, BA_FUNCTIONS_CREATE_RESULTS* results)
{
    // The module allocates while it starts. WixStdBA is C++ code that does not expect exceptions, so
    // a failed allocation becomes an error code here.
    try {
        auto module = std::make_unique<Varjo::Module>(Varjo::g_instance, args->pEngine);
        const HRESULT hr = module->createBAFunctions(*args, *results);
        if (SUCCEEDED(hr)) {
            Varjo::g_module = module.release();
        }
        return hr;
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    }
}

extern "C" void WINAPI BAFunctionsDestroy(const BA_FUNCTIONS_DESTROY_ARGS*, BA_FUNCTIONS_DESTROY_RESULTS*)
{
    const std::unique_ptr<Varjo::Module> module(Varjo::g_module);
    Varjo::g_module = nullptr;
}

namespace Varjo
{
namespace
{

BalSession::BalSession(IBootstrapperEngine* engine) { BalInitialize(engine); }

BalSession::~BalSession() { BalUninitialize(); }

GdiplusSession::GdiplusSession()
{
    const Gdiplus::GdiplusStartupInput input;
    m_running = Gdiplus::GdiplusStartup(&m_token, &input, nullptr) == Gdiplus::Ok;
}

GdiplusSession::~GdiplusSession()
{
    if (m_running) {
        Gdiplus::GdiplusShutdown(m_token);
    }
}

bool GdiplusSession::isRunning() const { return m_running; }

Module::Module(HINSTANCE instance, IBootstrapperEngine* engine)
    : m_instance(instance)
    , m_bal(engine)
    , m_dimmerClass(Dimmer::windowClass(instance))
    , m_choiceDialogClass(choiceDialogWindowClass(instance))
    , m_images(instance)
{
}

HRESULT Module::createBAFunctions(const BA_FUNCTIONS_CREATE_ARGS& args, BA_FUNCTIONS_CREATE_RESULTS& results)
{
    if (!m_gdiplus.isRunning()) {
        return E_FAIL;
    }
    m_functions.reset(new (std::nothrow) VarjoBAFunctions(m_instance, m_images));
    if (!m_functions) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = m_functions->OnCreate(args.pEngine, args.pCommand);
    if (FAILED(hr)) {
        m_functions.reset();
        return hr;
    }
    results.pfnBAFunctionsProc = varjoBAFunctionsProc;
    results.pvBAFunctionsProcContext = m_functions.get();
    return S_OK;
}

// WixStdBA passes every event to the DLL through this function. The events of the engine have the
// numbers below the first message of the theme.
HRESULT WINAPI varjoBAFunctionsProc(BA_FUNCTIONS_MESSAGE message, const LPVOID args, LPVOID results, LPVOID context)
{
    const HRESULT hr = BalBaseBAFunctionsProc(message, args, results, context);
    if (message < BA_FUNCTIONS_MESSAGE_ONTHEMELOADED) {
        static_cast<VarjoBAFunctions*>(context)->finishEngineEvent();
    }
    return hr;
}

}  // namespace
}  // namespace Varjo
