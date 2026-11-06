#ifndef MODULE_H
#define MODULE_H

#define MAX_INCLUDE_PATHS 100

struct module_lookup {
    int num_include_paths;
    const char *include_paths[MAX_INCLUDE_PATHS];
};

extern struct module_lookup module_paths;

void add_include_path(const char *p);

#endif
