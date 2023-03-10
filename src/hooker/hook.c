#include <hooker/hook.h>


//////////////////////////// HOOK API /////////////////////////
bool hook_func(void *hook_func, void *detour_func, void *original_func)
{
    if (MH_CreateHook(hook_func, detour_func, (LPVOID *)original_func) != MH_OK)
    {
        printf("Failed to create func hook, RVA: %llu \n", (uintptr_t)hook_func);
        return false;
    }
    return true;
}

//////////////////////////// SYM API /////////////////////////
void *get_rva_func(unsigned int rva)
{
    uintptr_t base_addr = (uintptr_t)GetModuleHandle(NULL);
    return (void *)(base_addr + rva + 4096);
}

void split_rva_val_line(const char *line, const char *sym, unsigned int *rva_val)
{
    char *split_str = malloc(strlen(line));
    strncpy(split_str, line, strlen(line));

    char *rva_val_str = strtok(split_str, ":");
    rva_val_str = strtok(NULL, ", ");
    sscanf(rva_val_str, "[%*x:%x]", rva_val);
    free(split_str);
}

void *dlsym(const char *sym)
{
    static bool is_sym_file_generated = false;
    unsigned int rva_val = 0;

    rva_val = get_rva_from_hashmap(sym);
    if (rva_val == -1)
    {
        load_hashmap_from_file(SYM_CACHE_FILE);
        rva_val = get_rva_from_hashmap(sym);
        if (rva_val != -1)
            return get_rva_func(rva_val);
    }
    else
    {
        return get_rva_func(rva_val);
    }
    FILE *fp = fopen(SYM_FILE, "r");
    if (!fp)
    {
        if (!release_cvdump_exe())
            return NULL;

        printf("Symbol file " SYM_FILE " not found, trying to generate.\n");

        system(CVDUMP_EXE_PATH CVDUMP_EXEC_ARGS BDS_PDB_PATH " > " SYM_FILE );

        fp = fopen(SYM_FILE, "r");
        if (!fp)
        {
            if (!is_sym_file_generated)
            {
                printf("Failed to generate. \n");
                is_sym_file_generated = true;
            }
            printf("The symbol %s will not be parsed. \n", sym);
            return NULL;
        }
    }

    const size_t MAX_LINE_LENGTH = 4096;
    char *line = (char*)malloc(MAX_LINE_LENGTH);

    if (!line)
        return NULL;

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL)
    {
        if (strstr(line, sym))
        {
            split_rva_val_line(line, sym, &rva_val);
            break;
        }
    }
    
    add_entry(sym, rva_val);
    save_hashmap_to_file(SYM_CACHE_FILE);

    free(line);
    fclose(fp);

    return get_rva_func(rva_val);
}

bool release_cvdump_exe(void)
{
    FILE* fp = fopen(CVDUMP_EXE_PATH, "wb");
    if (fp)
    {
        fwrite(CVDUMP_EXE_RES, sizeof(CVDUMP_EXE_RES), 1, fp);
        printf("Release resource cvdump.exe success.\n");
        fclose(fp);
        return true;
    }
    else
    {
        printf("Release resource cvdump.exe failed.\n");
        return false;
    }
}

//////////////////// MinHook ////////////////////

bool hooker_init(void)
{
    load_hashmap_from_file(SYM_CACHE_FILE);
    if (MH_Initialize() != MH_OK)
        return false;
        
    return true;
}

bool hooker_uninit(void)
{
    save_hashmap_to_file(SYM_CACHE_FILE);
    free_hashmap();
    if (MH_Uninitialize() != MH_OK)
        return false;
        
    return true;
}

bool hooker_enable_all_hook(void)
{
    if (MH_QueueEnableHook(MH_ALL_HOOKS) != MH_OK)
        return false;
    if (MH_ApplyQueued() != MH_OK)
        return false;

    return true;
}

bool hooker_disable_all_hook(void)
{
    if (MH_QueueDisableHook(MH_ALL_HOOKS)!= MH_OK)
        return false;
    if (MH_ApplyQueued()!= MH_OK)
        return false;

    return true;
}
