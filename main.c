#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    void *handle = dlopen("./foo.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    char *foo = dlsym(handle, "foo");
    if (!foo) {
        fprintf(stderr, "dlsym: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    puts(foo); // Prints "bar"
}
