#include <string.h>
#include "module.h"

struct module_lookup module_paths = {0};

void add_include_path(const char *p)
{
    module_paths.include_paths[module_paths.num_include_paths++] = strdup(p);
}

void add_imported_file(const char *p)
{
    if (has_file_been_imported(p)) return;
    module_paths.imported_files[module_paths.num_imported_files++] = strdup(p);
}

int has_file_been_imported(const char *p)
{
    for (int i = 0; i < module_paths.num_imported_files; ++i)
        if (!strcmp(module_paths.imported_files[i], p)) return 1;
    return 0;
}
