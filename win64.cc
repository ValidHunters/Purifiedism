/*
REM c++20 is for fmt, cba
cl /nologo /EHsc /O2 /std:c++20 /utf-8 win64.cc /LD /MD
dotnet build ./Matsuoka/Matsuoka.csproj
copy /Y "Matsuoka\bin\Debug\net8.0\Matsuoka.dll" "%TEMP%\Matsuoka.dll"
*/

/*
Some advantages of Windows:
 * Manual mapping is ez af without 9001 funny arguments and wonky ptrace "scripts".
   * C++ also doesn't make your shit die like LD_PRELOAD.
 * PDBs on Microsoft servers for all first-party apps including dotnet.
   * This eliminates all the guesswork as I can just parse structs and globals.
 * I am more familiar with Windows on AMD64.
   * MSABI aka MS fastcall
   * Almost nothing statically links.
   * Sometimes asserts are present in release apps.
   * Superiour x64dbg.
*/

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <thread>
#include <iostream>
#include <chrono>
#include <format>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <psapi.h>

#include "memory.hh"

using coreclr_create_delegate_fn = uint32_t(*)(void*, uint32_t, const char*, const char*, const char*, void**);

typedef WCHAR char_t;

// Signature of delegate returned by coreclr_delegate_type::load_assembly_and_get_function_pointer
typedef int (*load_assembly_and_get_function_pointer_fn)(
    const char_t *assembly_path      /* Fully qualified path to assembly */,
    const char_t *type_name          /* Assembly qualified type name */,
    const char_t *method_name        /* Public static method name compatible with delegateType */,
    const char_t *delegate_type_name /* Assembly qualified delegate type name or null
                                        or UNMANAGEDCALLERSONLY_METHOD if the method is marked with
                                        the UnmanagedCallersOnlyAttribute. */,
    void         *reserved           /* Extensibility parameter (currently unused and must be 0) */,
    /*out*/ void **delegate          /* Pointer where to store the function pointer result */);

// Signature of delegate returned by load_assembly_and_get_function_pointer_fn when delegate_type_name == null (default)
typedef int (*component_entry_point_fn)(void *arg, int32_t arg_size_in_bytes);

static_assert(sizeof(wchar_t) == 2, "cl.EXE machine broke");

void main_thread() {
    // Disable buffering, even tho endl flushes
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    //uintptr_t host_policy = 0;
    while (!GetModuleHandleA("hostpolicy.dll")) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        //host_policy = GetModuleHandleA("hostpolicy.dll");
    }
    uintptr_t g_context_ptr = memory::dereference(memory::occurence("hostpolicy.dll", "48 89 3D ?? ?? ?? ?? 48 89 3D"), 3);
    while (!*(uintptr_t*)g_context_ptr) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    uintptr_t g_context = *(uintptr_t*)g_context_ptr;
    std::cerr << std::format("Found g_context @ {}", PVOID(g_context)) << std::endl;

    uintptr_t g_coreclr = 0;
    while (!(g_coreclr = *(uintptr_t*)(g_context + 0x110))) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cerr << std::format("Found g_coreclr @ {}", PVOID(g_coreclr)) << std::endl;

    auto host_handle = *(void**)(g_coreclr + 0x58);
    uint32_t domain_id = *(uint32_t*)(g_coreclr + 0x60);
    std::cerr << std::format("g_coreclr->{{host_handle, domain_id}} @ {} {}", PVOID(host_handle), domain_id) << std::endl;

    auto libcoreclr = GetModuleHandleA("coreclr.dll");
    auto coreclr_create_delegate = coreclr_create_delegate_fn(GetProcAddress(libcoreclr, "coreclr_create_delegate"));
    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
    auto hr = coreclr_create_delegate(
        host_handle,
        domain_id,
        "System.Private.CoreLib",
        "Internal.Runtime.InteropServices.ComponentActivator",
        "LoadAssemblyAndGetFunctionPointer",
        (void**)&load_assembly_and_get_function_pointer
    );
    std::cerr << std::format("load_assembly_and_get_function_pointer {:#x} {}", hr, (void*)load_assembly_and_get_function_pointer) << std::endl;

    if (hr || !load_assembly_and_get_function_pointer) {
        return;
    }

    // Fnnuy
    // 2047 should be enough.
    WCHAR buf[2048];
    ExpandEnvironmentStringsW(L"%TEMP%\\Matsuoka.dll", buf, _countof(buf));
    //std::cerr << std::format("Using path `{}` for loading", buf) << std::endl;
    component_entry_point_fn delegate = nullptr;
    hr = load_assembly_and_get_function_pointer(
        buf,
        L"Matsuoka.Seiko, Matsuoka",
        L"Kickstart",
        nullptr,
        nullptr,
        (void**)&delegate
    );
    std::cerr << std::format("component_entry_point_fn @ {} {:#x}", (void*)delegate, hr) << std::endl;
    
    if (delegate) {
        hr = delegate(nullptr, 0);
        std::cerr << std::format("delegate = {:#x}", hr) << std::endl;
    }
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved )  // reserved
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        std::thread(main_thread).detach(); // Could've used winapi but blergh
    }
    return TRUE;
}
