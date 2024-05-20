#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void print() {
    void *handle = dlopen("./foo.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    // char *foo = dlsym(handle, "foo");
    // if (!foo) {
    //     fprintf(stderr, "dlsym: %s", dlerror());
    //     exit(EXIT_FAILURE);
    // }

    // puts(foo); // Prints "bar"

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
