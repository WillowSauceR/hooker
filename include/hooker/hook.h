#pragma once
#pragma comment(lib, "minhook.x64d.lib")
#include "minhook/MinHook.h"
#include "cvdump_exe_res.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define SYM_FILE "bedrock_server_sym.txt"
#define SYM_CACHE_FILE "bedrock_server_sym_cache.bin"

#define CVDUMP_URL "https://raw.github.com/microsoft/microsoft-pdb/master/cvdump/cvdump.exe"
#define CVDUMP_EXE_PATH "cvdump.exe"
#define CVDUMP_EXEC_ARGS " -headers -p "
#define CVDUMP_MISSING_MSG \
            CVDUMP_EXE_PATH " not found, please download it from " CVDUMP_URL \
            " and then put it in the same level folder as bedrock_server.pdb\n"

#define BDS_PDB_PATH "bedrock_server.pdb"

#define TMHOOK(name, ret_type, rva_OR_sym, ...)             \
    typedef ret_type (*_##name##_t)(__VA_ARGS__);           \
    _##name##_t _original_##name = NULL;                    \
    typedef struct _##name _##name##_struct;                \
    struct _##name                                          \
    {                                                       \
        _##name##_t hook;                                   \
        _##name##_t original;                               \
        _##name##_t detour;                                 \
        bool (*init)(_##name##_struct*);                    \
        bool (*disable)(_##name##_struct*);                 \
        bool (*enable)(_##name##_struct*);                  \
        bool (*remove)(_##name##_struct*);                  \
    };                                                      \
    ret_type _detour_##name(__VA_ARGS__);                   \
    bool _INIT_HOOK_##name(_##name##_struct *name)          \
    {                                                       \
        void *func_ptr = atoi(rva_OR_sym)                   \
                        ? get_rva_func(atoi(rva_OR_sym))    \
                        : dlsym(rva_OR_sym);                \
        _##name##_t _hook_##name =                          \
                        (_##name##_t)func_ptr;              \
        bool result = hook_func(_hook_##name,               \
                                _detour_##name,             \
                                &_original_##name);         \
        name->hook = _hook_##name;                          \
        name->original = _original_##name;                  \
        name->detour = _detour_##name;                      \
        return result;                                      \
    }                                                       \
    bool _DISABLE_HOOK_##name(_##name##_struct *name)       \
    {                                                       \
        return MH_DisableHook(name->hook) == MH_OK          \
                    ? true : false;                         \
    }                                                       \
    bool _ENABLE_HOOK_##name(_##name##_struct *name)        \
    {                                                       \
        return MH_EnableHook(name->hook) == MH_OK           \
                    ? true : false;                         \
    }                                                       \
    bool _REMOVE_HOOK_##name(_##name##_struct *name)        \
    {                                                       \
        return MH_RemoveHook(name->hook) == MH_OK           \
                    ? true : false;                         \
    }                                                       \
    _##name##_struct name =                                 \
    {                                                       \
        NULL,                                               \
        NULL,                                               \
        NULL,                                               \
        _INIT_HOOK_##name,                                  \
        _DISABLE_HOOK_##name,                               \
        _ENABLE_HOOK_##name,                                \
        _REMOVE_HOOK_##name                                 \
    };                                                      \
    ret_type _detour_##name(__VA_ARGS__)


#define TMCALL(rva_OR_sym, func_proto, ...)                 \
    ((func_proto)                                           \
    (atoi(rva_OR_sym)                                       \
        ? get_rva_func(atoi(rva_OR_sym))                    \
        : dlsym(rva_OR_sym)))                               \
    (__VA_ARGS__)


#define DEREFERENCE(type, ptr, offset)                      \
    (*(type*)((uintptr_t)ptr + offset))


// for uintptr_t
#define PTR_OFFEST(ptr, offset)                             \
    ((uintptr_t)ptr + offset)


#define VIRTUAL_CALL(ret_type, ptr, offset, ...)            \
    (*(ret_type(**)(void*, ##__VA_ARGS__))                  \
    (DEREFERENCE(uintptr_t, ptr, offset)))                  \
    (ptr, ##__VA_ARGS__)


#ifdef __cplusplus
extern "C" {
#endif

void save_sym_cache();
void load_sym_cache();
bool hook_func(void *hook_func, void *detour_func, void *original_func);
void *get_rva_func(unsigned int rva);
void *dlsym(const char *sym);
bool release_cvdump_exe(void);

bool hooker_init(void);
bool hooker_uninit(void);
bool hooker_enable_all_hook(void);
bool hooker_disable_all_hook(void);

#ifdef __cplusplus
}
#endif
