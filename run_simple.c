#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void print() {
    void *handle = dlopen("./simple.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    char *a = dlsym(handle, "a");
    if (!a) {
        fprintf(stderr, "dlsym: %s", dlerror());
        exit(EXIT_FAILURE);
    }
    puts(a); // Prints "a^"
}

int main() {
    print();
}
