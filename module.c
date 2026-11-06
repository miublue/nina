#include "module.h"

struct module_lookup module_paths = {0};

void add_include_path(const char *p)
{
    module_paths.include_paths[module_paths.num_include_paths++] = p;
}

