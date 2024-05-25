#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void print() {
    void *handle = dlopen("./full.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    // TODO: Put this back
    // char *a = dlsym(handle, "a");
    // if (!a) {
    //     fprintf(stderr, "dlsym: %s", dlerror());
    //     exit(EXIT_FAILURE);
    // }
    // puts(a); // Prints "a^"

    int (*fn_a)(void) = dlsym(handle, "fn_a");
    if (!fn_a) {
        fprintf(stderr, "dlsym: %s", dlerror());
        exit(EXIT_FAILURE);
    }
    printf("%d\n", fn_a());
}

int main() {
    print();
}
