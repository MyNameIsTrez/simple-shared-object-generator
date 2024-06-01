#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct define {
    uint16_t a;
    uint8_t b;
};

void print() {
    void *handle = dlopen("./full.so", RTLD_NOW);
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

    int (*fn_a)(void) = dlsym(handle, "fn1_c");
    if (!fn_a) {
        fprintf(stderr, "dlsym: %s", dlerror());
        exit(EXIT_FAILURE);
    }
    printf("%d\n", fn_a());

    void *define_void = dlsym(handle, "define");
    if (!define_void) {
        fprintf(stderr, "dlsym: %s", dlerror());
        exit(EXIT_FAILURE);
    }
    struct define d = *(struct define *)define_void;
    printf("%d\n", d.a);
    printf("%d\n", d.b);
}

int main() {
    print();
}
