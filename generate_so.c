#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum e_type {
    ET_DYN = 3, // Shared object
};

enum p_type {
    PT_LOAD = 1, // Loadable segment
    PT_DYNAMIC = 2, // Dynamic linking information
};

enum p_flags {
    PF_W = 2, // Writable segment
    PF_R = 4, // Readable segment
};

enum sh_type {
    SHT_PROGBITS = 0x1, // Program data
    SHT_SYMTAB = 0x2, // Symbol table
    SHT_STRTAB = 0x3, // String table
    SHT_HASH = 0x5, // Symbol hash table
    SHT_DYNAMIC = 0x6, // Dynamic linking information
    SHT_DYNSYM = 0xb, // Dynamic linker symbol table
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

static void push_shstrtab() {
    push(0);
    push_string(".symtab");
    push_string(".strtab");
    push_string(".shstrtab");
    push_string(".hash");
    push_string(".dynsym");
    push_string(".dynstr");
    push_string(".eh_frame");
    push_string(".dynamic");
    push_string(".data");
    push_zeros(2);
}

static void push_strtab() {
    push(0);
    push_string("foo.s");
    push_string("_DYNAMIC");
    push_string("foo");
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
    // 0x20e0 to 0x2120
    push_zeros(0x40);

    // .hash: Hash section
    // 0x2120 to 0x2160
    push_section(
        0x1b,
        SHT_HASH,
        SHF_ALLOC,
        0x120,
        0x120,
        0x14,
        2,
        0,
        8,
        4
    );

    // .dynsym: Dynamic linker symbol table section
    // 0x2160 to 0x21a0
    push_section(
        0x21,
        SHT_DYNSYM,
        SHF_ALLOC,
        0x138,
        0x138,
        0x30,
        3,
        1,
        8,
        0x18
    );

    // .dynstr: String table section
    // 0x21a0 to 0x21e0
    push_section(
        0x29,
        SHT_STRTAB,
        SHF_ALLOC,
        0x168,
        0x168,
        5,
        0,
        0,
        1,
        0
    );

    // .eh_frame: Program data section
    // 0x21e0 to 0x2220
    push_section(
        0x31,
        SHT_PROGBITS,
        SHF_ALLOC,
        0x1000,
        0x1000,
        0,
        0,
        0,
        8,
        0
    );

    // .dynamic: Dynamic linking information section
    // 0x2220 to 0x2260
    push_section(
        0x3b,
        SHT_DYNAMIC,
        SHF_WRITE | SHF_ALLOC,
        0x1f50,
        0x1f50,
        0xb0,
        3,
        0,
        8,
        0x10
    );

    // .data: Data section
    // 0x2260 to 0x22a0
    push_section(
        0x44,
        SHT_PROGBITS,
        SHF_WRITE | SHF_ALLOC,
        0x2000,
        0x2000,
        4,
        0,
        0,
        4,
        0
    );

    // .symtab: Symbol table section
    // 0x22a0 to 0x22e0
    push_section(
        1,
        SHT_SYMTAB,
        0,
        0,
        0x2008,
        0x78,
        8, // Section header index of the associated string table; see https://blog.k3170makan.com/2018/09/introduction-to-elf-file-format-part.html
        4, // One greater than the symbol table index of the last local symbol (binding STB_LOCAL)
        8,
        0x18
    );

    // .strtab: String table section
    // 0x22e0 to 0x2320
    push_section(
        0x09,
        SHT_PROGBITS | SHT_SYMTAB,
        0,
        0,
        0x2080,
        0x14,
        0,
        0,
        1,
        0
    );

    // .shstrtab: Section header string table section
    // 0x2320 to end
    push_section(
        0x11,
        SHT_PROGBITS | SHT_SYMTAB,
        0,
        0,
        0x2094,
        0x4a,
        0,
        0,
        1,
        0
    );
}

static void push_program_header(u32 type, u32 flags, u64 offset, u64 virtual_address, u64 physical_address, u64 file_size, u64 mem_size, u64 alignment) {
    push_number(type, 4);
    push_number(flags, 4);
    push_number(offset, 8);
    push_number(virtual_address, 8);
    push_number(physical_address, 8);
    push_number(file_size, 8);
    push_number(mem_size, 8);
    push_number(alignment, 8);
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

    // Shared object
    push(ET_DYN);
    push(0);

    // x86-64 instruction set architecture
    push(0x3E);
    push(0);

    // Original version of ELF
    push(1);
    push_zeros(3);

    // No execution entry point address
    push_zeros(8);

    // Program header table offset
    push(0x40);
    push_zeros(7);

    // Section header table offset
    push(0xe0);
    push(0x20);
    push_zeros(6);

    // Processor-specific flags
    push_zeros(4);

    // ELF header size
    push(0x40);
    push(0);

    // Single program header size
    push(0x38);
    push(0);

    // Number of program header entries
    push(4);
    push(0);

    // Single section header entry size
    push(0x40);
    push(0);

    // Number of section header entries
    push(10);
    push(0);

    // Index of entry with section names
    push(9);
    push(0);
}

static void generate_so() {
    FILE *f = fopen("foo.so", "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // 0x0 to 0x40
    push_elf_header();

    // 0x40 to 0x120
    push_program_header(PT_LOAD, PF_R, 0, 0, 0, 0x1000, 0x1000, 0x1000);
    push_program_header(PT_LOAD, PF_R | PF_W, 0x1f50, 0x1f50, 0x1f50, 0xb4, 0xb4, 0x1000);
    push_program_header(PT_DYNAMIC, PF_R | PF_W, 0x1f50, 0x1f50, 0x1f50, 0xb0, 0xb0, 8);
    push_program_header(0x6474e552, PF_R, 0x1f50, 0x1f50, 0x1f50, 0xb0, 0xb0, 1);

    // TODO: REMOVE!
    push_zeros(0x1ee0);

    // 0x2000 to 0x2004
    push_data();

    // TODO: REMOVE!
    push_zeros(0x70);

    // 0x2080 to 0x2094
    push_strtab();

    // 0x2094 to 0x20e0
    push_shstrtab();

    // 0x20e0 to end
    push_section_headers();

    // TODO: ? to TODO: ?
    // push_symbol_table();

    fwrite(bytes, sizeof(u8), bytes_size, f);

    fclose(f);
}

int main() {
    generate_so();
}
