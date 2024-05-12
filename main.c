#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void print() {
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

uint8_t bytes[420420];
size_t bytes_len = 0;

static void push(uint8_t byte) {
    bytes[bytes_len++] = byte;
}

static void push_zeros(size_t count) {
    for (size_t i = 0; i < count; i++) {
        push(0);
    }
}

static void push_elf_file_header() {
    // Magic number
    push(0x7f);
    push('E');
    push('L');
    push('F');

    // 64-bit
    push(2);

    // Little-endian
    push(1);

    // Version
    push(1);

    // SysV OS ABI
    push(0);

    // Padding
    push_zeros(8);

    // Relocatable file
    push(1);
    push(0);

    // x86-64 instruction set architecture
    push(0x3E);
    push(0);

    // Original version of ELF
    push(1);
    push_zeros(3);

    // No execution entry point address
    push_zeros(8);

    // No program header table
    push_zeros(8);

    // Section header table offset
    push(0x40);
    push_zeros(7);

    // Processor-specific flags
    push_zeros(4);

    // ELF header size
    push(0x40);
    push(0);

    // Single program header size
    push_zeros(2);

    // Number of program header entries
    push_zeros(2);

    // Single section header entry size
    push(0x40);
    push(0);

    // Number of section header entries
    push(5);
    push(0);

    // Index of entry with section names
    push(2);
    push(0);
}

static void generate_so() {
    FILE *f = fopen("foo.o", "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    push_elf_file_header();

    fwrite(bytes, sizeof(uint8_t), bytes_len, f);

    fclose(f);
}

int main() {
    generate_so();

    // print();
}
