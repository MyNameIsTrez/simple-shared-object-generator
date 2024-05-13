#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum e_type {
    ET_REL = 1, // Relocatable file
};

enum sh_type {
    SHT_PROGBITS = 1, // Program data
    SHT_SYMTAB = 2, // Symbol table
};

enum sh_flags {
    SHF_WRITE = 1, // Writable
    SHF_ALLOC = 2, // Occupies memory during execution
};

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define MAX_BYTES_SIZE 420420
u8 bytes[MAX_BYTES_SIZE];
size_t bytes_size = 0;

static void push(u8 byte) {
    if (bytes_size + 1 > MAX_BYTES_SIZE) {
        fprintf(stderr, "MAX_BYTES_SIZE of %d was exceeded\n", MAX_BYTES_SIZE);
        exit(EXIT_FAILURE);
    }

    bytes[bytes_size++] = byte;
}

static void push_zeros(size_t count) {
    for (size_t i = 0; i < count; i++) {
        push(0);
    }
}

static void push_string(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        push(str[i]);
    }
    push('\0');
}

static void push_symbol_entry_names() {
    push(0);
    push_string("foo.s");
    push_string("foo");
    push_zeros(5);
}

// TODO: I don't know the algorithm behind this
static void push_symbol_table() {
    push_zeros(0x10);

    push_zeros(8);

    // Not sure if this is two groups of four bytes,
    // or one group of eight bytes
    push(1);
    push(0);
    push(0);
    push(0);
    push(4);
    push(0);
    push(0xf1);
    push(0xff);

    push_zeros(0x10);

    push_zeros(4);
    push(3);
    push(0);
    push(1);
    push_zeros(9);

    push_zeros(8);
    push(7);
    push(0);
    push(0);
    push(0);
    push(0x10);
    push(0);
    push(1);
    push(0);

    push_zeros(0x10);
}

static void push_section_names() {
    push(0);
    push_string(".data");
    push_string(".shstrtab");
    push_string(".symtab");
    push_string(".strtab");
    push_zeros(15);
}

static void push_data() {
    push_string("bar");
    push_zeros(12);
}

static void push_number(u64 n, size_t byte_count) {
    while (n > 0) {
        // Little-endian requires the least significant byte first
        push(n & 0xff);
        byte_count--;

        n >>= 8; // Shift right by one byte
    }

    // Optional padding
    push_zeros(byte_count);
}

static void push_section(u32 name_offset, u32 type, u64 flags, u64 address, u64 offset, u64 size, u32 link, u32 info, u64 alignment, u64 entry_size) {
    push_number(name_offset, 4);
    push_number(type, 4);
    push_number(flags, 8);
    push_number(address, 8);
    push_number(offset, 8);
    push_number(size, 8);
    push_number(link, 4);
    push_number(info, 4);
    push_number(alignment, 8);
    push_number(entry_size, 8);
}

static void push_section_headers() {
    // Null section
    // 0x40 to 0x80
    push_zeros(0x40);

    // Data section
    // 0x80 to 0xc0
    push_section(
        0x01,
        SHT_PROGBITS,
        SHF_WRITE | SHF_ALLOC,
        0,
        0x180,
        4,
        0,
        0,
        4,
        0
    );

    // Names section
    // 0xc0 to 0x100
    push_section(
        0x07,
        SHT_PROGBITS | SHT_SYMTAB,
        0,
        0,
        0x190,
        0x21,
        0,
        0,
        1,
        0
    );

    // Symbol table section
    // 0x100 to 0x140
    push_section(
        0x11,
        SHT_SYMTAB,
        0,
        0,
        0x1c0,
        0x60,
        4, // Section header index of the associated string table; see https://blog.k3170makan.com/2018/09/introduction-to-elf-file-format-part.html
        3, // One greater than the symbol table index of the last local symbol (binding STB_LOCAL)
        8,
        0x18
    );

    // Symbol entry names section
    // 0x140 to 0x180
    push_section(
        0x19,
        SHT_PROGBITS | SHT_SYMTAB,
        0,
        0,
        0x220,
        0x0b,
        0,
        0,
        1,
        0
    );
}

static void push_elf_header() {
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
    push(ET_REL);
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

static void generate_o() {
    FILE *f = fopen("foo.o", "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // 0x0 to 0x40
    push_elf_header();

    // 0x40 to 0x180
    push_section_headers();

    // 0x180 to 0x190
    push_data();

    // 0x190 to 0x1b1
    push_section_names();

    // 0x1c0 to 0x220
    push_symbol_table();

    // 0x220 to end
    push_symbol_entry_names();

    fwrite(bytes, sizeof(u8), bytes_size, f);

    fclose(f);
}

int main() {
    generate_o();
}
