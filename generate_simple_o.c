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

enum st_binding {
    STB_LOCAL = 0, // Local symbol
    STB_GLOBAL = 1, // Global symbol
};

enum st_type {
    STT_NOTYPE = 0, // The symbol type is not specified
    STT_SECTION = 3, // This symbol is associated with a section
    STT_FILE = 4, // This symbol is associated with a file
};

enum sh_index {
    SHN_UNDEF = 0, // An undefined section reference
    SHN_ABS = 0xfff1, // Absolute values for the corresponding reference
};

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// From "st_info" its description here:
// https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-79797/index.html
#define ELF32_ST_INFO(bind, type) (((bind)<<4)+((type)&0xf))

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

static void push_string(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        push(str[i]);
    }
    push('\0');
}

static void push_strtab() {
    push(0);
    push_string("simple.s");
    push_string("a");
    push_zeros(4);
}

// See https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-79797/index.html
// See https://docs.oracle.com/cd/E19683-01/816-1386/6m7qcoblj/index.html#chapter6-tbl-21
static void push_symbol(u32 name, u16 info, u16 shndx) {
    push_number(name, 4); // Indexed into .strtab, because .symtab its "link" points to it
    push_number(info, 2);
    push_number(shndx, 2);

    // TODO: I'm confused by why we don't seem to need these
    // push_number(value, 4);
    // push_number(size, 4);
    // push_number(other, 4);

    push_zeros(24 - 8); // .symtab its entry_size is 24
}

static void push_symtab() {
    // Null entry
    // 0x1c0 to 0x1d8
    push_symbol(0, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), SHN_UNDEF);

    // "simple.s" entry
    // 0x1d8 to 1x1f0
    push_symbol(1, ELF32_ST_INFO(STB_LOCAL, STT_FILE), SHN_ABS);

    // TODO: ? entry
    // 1x1f0 to 0x208
    push_symbol(0, ELF32_ST_INFO(STB_LOCAL, STT_SECTION), 1);

    // "a" entry
    // 0x208 to 0x220
    push_symbol(10, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 1);
}

static void push_shstrtab() {
    push(0);
    push_string(".data");
    push_string(".shstrtab");
    push_string(".symtab");
    push_string(".strtab");
    push_zeros(15);
}

static void push_data() {
    push_string("a^");
    push_zeros(13);
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

    // .data: Data section
    // 0x80 to 0xc0
    push_section(1, SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, 0, 0x180, 3, 0, 0, 4, 0);

    // .shstrtab: Section header string table section
    // 0xc0 to 0x100
    push_section(7, SHT_PROGBITS | SHT_SYMTAB, 0, 0, 0x190, 33, 0, 0, 1, 0);

    // .symtab: Symbol table section
    // 0x100 to 0x140
    // The "link" of 4 is the section header index of the associated string table, so .strtab
    // The "info" of 3 is the symbol table index of the first non-local symbol, which is the 4th entry in push_symtab(), the global "a" symbol
    push_section(17, SHT_SYMTAB, 0, 0, 0x1c0, 96, 4, 3, 8, 24);

    // .strtab: String table section
    // 0x140 to 0x180
    push_section(25, SHT_PROGBITS | SHT_SYMTAB, 0, 0, 0x220, 12, 0, 0, 1, 0);
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

static void generate_simple_o() {
    FILE *f = fopen("simple.o", "w");
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
    push_shstrtab();

    // 0x1c0 to 0x220
    push_symtab();

    // 0x220 to end
    push_strtab();

    fwrite(bytes, sizeof(u8), bytes_size, f);

    fclose(f);
}

int main() {
    generate_simple_o();
}
