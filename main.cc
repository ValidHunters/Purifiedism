/*
clang++ -shared -fPIC -fshort-wchar -ldl -Wall -pedantic main.cc -o SS14.Reloader.so
dotnet build ./Matsuoka/Matsuoka.csproj
cp ./Matsuoka/bin/Debug/net8.0/Matsuoka.dll /tmp/Matsuoka.dll
*/

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <thread>
#include <iostream>
#include <chrono>

#include <errno.h>
#include <dlfcn.h>
// #include <pthread.h>

#include "memory.hh"

using coreclr_create_delegate_fn = uint32_t(*)(void*, uint32_t, const char*, const char*, const char*, void**);

typedef char char_t;

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

static_assert(sizeof(wchar_t) == 2, "-fshort-wchar");

void main_thread() {
    // std::cerr << "I AM ALIVE!" << std::endl;
    //fputs("I AM ALIVE!\n", stderr);

    bool flag = true;
    while (flag) {
        auto f = fopen("/proc/self/maps", "r");
        size_t line_size = 0;
        char* line = nullptr;
        while(getline(&line, &line_size, f) != -1) {
            if (strstr(line, "libhostpolicy.so")) {
                // we got the shoes
                auto addr_begin = strtoull(strtok(line, "-"), NULL, 16);
                auto addr_end = strtoull(strtok(NULL, " "), NULL, 16);
                auto perms = strtok(NULL, " ");
                // we got the money
                std::fprintf(stderr, "Perms %s\n", perms);

                if(perms[2] == 'x') {
                    auto ptr = memory::occurence(addr_begin, addr_end, "0F 11 05 ?? ?? ?? ?? 48 85 DB");
                    //auto ptr = memory::occurence(addr_begin, addr_end, "7f 69 aa bc 40 00");
                    std::fprintf(stderr, "Sigscan %p %p %p\n", addr_begin, addr_end, ptr);

                    if (ptr) {
                        ptr = *(uintptr_t*)memory::dereference(ptr, 3);
                        if (ptr) {
                            std::fprintf(stderr, "Found g_context @ %p\n", (void*)ptr);
                            while (*(uintptr_t*)(ptr + 0x100) == 0) {
                                std::this_thread::sleep_for(std::chrono::seconds(1));

                                //std::cerr << "CoreCLR isn't initialized yet..." << std::endl;

                                fputs("CoreCLR isn't initialized yet...\n", stderr);
                            }
                            auto coreclr = *(uintptr_t*)(ptr + 0x100);
                            auto host_handle = *(void**)(coreclr + 0x30);
                            auto domain_id = *(uint32_t*)(coreclr + 0x38);
                            std::fprintf(stderr, "CoreCLR is initialized... (%p, %d)\n", host_handle, domain_id);
                            flag = false;
                            auto libcoreclr = dlopen("libcoreclr.so", RTLD_LAZY | RTLD_NOLOAD);
                            auto coreclr_create_delegate = coreclr_create_delegate_fn(dlsym(libcoreclr, "coreclr_create_delegate"));
                            //std::fprintf(stderr, "coreclr_create_delegate @ %p from %p (%p)\n", (void*)coreclr_create_delegate, libcoreclr, *(void**)libcoreclr);
                            if (coreclr_create_delegate) {
                                load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
                                auto hr = coreclr_create_delegate(
                                    host_handle,
                                    domain_id,
                                    "System.Private.CoreLib",
                                    "Internal.Runtime.InteropServices.ComponentActivator",
                                    "LoadAssemblyAndGetFunctionPointer",
                                    (void**)&load_assembly_and_get_function_pointer
                                );
                                std::fprintf(stderr, "load_assembly_and_get_function_pointer %d %p\n", hr, (void*)load_assembly_and_get_function_pointer);
                                if (!hr && load_assembly_and_get_function_pointer) {
                                    flag = false;

                                    component_entry_point_fn delegate = nullptr;
                                    auto hr = load_assembly_and_get_function_pointer(
                                        "/tmp/Matsuoka.dll",
                                        "Matsuoka.Seiko, Matsuoka",
                                        "Kickstart",
                                        nullptr,
                                        nullptr,
                                        (void**)&delegate
                                    );
                                    std::fprintf(stderr, "component_entry_point_fn @ %p %d\n", (void*)delegate, hr);
                                    if (!hr && delegate) {
                                        //*
                                        for (size_t i=0; i<100; i++) {
                                            std::fprintf(stderr, "%02X ", ((uint8_t*)delegate)[i]);
                                        }
                                        std::fprintf(stderr, "\n");
                                        // */

                                        auto hr = delegate(nullptr, 0);
                                        std::fprintf(stderr, "delegate @ %d\n", hr);

                                        // constexpr size_t TypeName__GetTypeFromAsmQualifiedName_offset = 0x3E2E90;
                                        // using TypeName__GetTypeFromAsmQualifiedName_fn = uintptr_t(*)(const wchar_t*, void*, int, int, int, void*, void*, void*);
                                        // auto TypeName__GetTypeFromAsmQualifiedName = TypeName__GetTypeFromAsmQualifiedName_fn(*(uintptr_t*)libcoreclr + TypeName__GetTypeFromAsmQualifiedName_offset);
                                        constexpr size_t MemberLoader__FindMethodByName_offset = 0x260610;
                                        using MemberLoader__FindMethodByName_fn = uintptr_t(*)(void*, void*, int);
                                        auto MemberLoader__FindMethodByName = MemberLoader__FindMethodByName_fn(*(uintptr_t*)libcoreclr + MemberLoader__FindMethodByName_offset);
                                        
                                        hr = coreclr_create_delegate(
                                            host_handle,
                                            domain_id,
                                            "Matsuoka",
                                            "Matsuoka.Seiko",
                                            "Kickstart",
                                            (void**)&delegate
                                        );
                                        std::fprintf(stderr, "component_entry_point_fn @ %p %08X\n", (void*)delegate, hr);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            free(line); line = nullptr;
        }
        fclose(f);
        if (flag) {
            // std::cerr << "Failed to find coreclr from libhostpolicy.so!" << std::endl;
            //fputs("Failed to find coreclr from libhostpolicy.so!\n", stderr);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    fputs("\033[35mGaming in 2023\033[0m\n", stderr);
}

void __attribute__((constructor)) init() {
    std::thread(main_thread).detach();
    // dlsym((void*)-1, "coreclr_initialize");
}