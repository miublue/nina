#ifndef MODULE_H
#define MODULE_H

#define MAX_INCLUDE_PATHS 100
#define MAX_IMPORT_FILES 1024

struct module_lookup {
    int num_include_paths, num_imported_files;
    const char *include_paths[MAX_INCLUDE_PATHS];
    const char *imported_files[MAX_IMPORT_FILES];
};

extern struct module_lookup module_paths;

void add_include_path(const char *p);
void add_imported_file(const char *p);
int has_file_been_imported(const char *p);

#endif
