#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum d_type {
    DT_NULL = 0, // Marks the end of the _DYNAMIC array
    DT_HASH = 4, // The address of the symbol hash table. This table refers to the symbol table indicated by the DT_SYMTAB element
    DT_STRTAB = 5, // The address of the string table
    DT_SYMTAB = 6, // The address of the symbol table
    DT_STRSZ = 10, // The total size, in bytes, of the DT_STRTAB string table
    DT_SYMENT = 11, // The size, in bytes, of the DT_SYMTAB symbol entry
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

enum e_type {
    ET_DYN = 3, // Shared object
};

enum st_binding {
    STB_LOCAL = 0, // Local symbol
    STB_GLOBAL = 1, // Global symbol
};

enum st_type {
    STT_NOTYPE = 0, // The symbol type is not specified
    STT_OBJECT = 1, // This symbol is associated with a data object
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
        fprintf(stderr, "error: MAX_BYTES_SIZE of %d was exceeded\n", MAX_BYTES_SIZE);
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
    push_zeros(1);
}

static void push_strtab() {
    push(0);
    push_string("simple.s");
    push_string("_DYNAMIC");
    push_string("a");
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

// See https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-79797/index.html
// See https://docs.oracle.com/cd/E19683-01/816-1386/6m7qcoblj/index.html#chapter6-tbl-21
static void push_symbol(u32 name, u16 info, u16 shndx, u32 value) {
    push_number(name, 4); // Indexed into .strtab, because .symtab its "link" points to it
    push_number(info, 2);
    push_number(shndx, 2);
    push_number(value, 4); // In executable and shared object files, st_value holds a virtual address

    // TODO: I'm confused by why we don't seem to need these
    // push_number(size, 4);
    // push_number(other, 4);

    push_zeros(24 - 12); // .symtab its entry_size is 24
}

static void push_symtab() {
    // Null entry
    // 0x2008 to 0x2020
    push_symbol(0, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), SHN_UNDEF, 0);

    // "simple.s" entry
    // 0x2020 to 0x2038
    push_symbol(1, ELF32_ST_INFO(STB_LOCAL, STT_FILE), SHN_ABS, 0);

    // TODO: ? entry
    // 0x2038 to 0x2050
    push_symbol(0, ELF32_ST_INFO(STB_LOCAL, STT_FILE), SHN_ABS, 0);

    // "_DYNAMIC" entry
    // 0x2050 to 0x2068
    push_symbol(10, ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 5, 0x1f50);

    // "a" entry
    // 0x2068 to 0x2080
    push_symbol(19, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 6, 0x2000);
}

static void push_data() {
    push_string("a^");
    push_zeros(5);
}

// See https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-42444.html
static void push_dynamic_entry(u64 tag, u64 value) {
    push_number(tag, 8);
    push_number(value, 8);
}

static void push_dynamic() {
    push_dynamic_entry(DT_HASH, 0x120);
    push_dynamic_entry(DT_STRTAB, 0x168);
    push_dynamic_entry(DT_SYMTAB, 0x138);
    push_dynamic_entry(DT_STRSZ, 3);
    push_dynamic_entry(DT_SYMENT, 24);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
}

static void push_dynstr() {
    push(0);
    push_string("a");
    push_zeros(5); // Alignment
}

static void push_dynsym() {
    // Null entry
    // 0x138 to 0x150
    push_symbol(0, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), SHN_UNDEF, 0);

    // "a" entry
    // 0x150 to 0x168
    push_symbol(1, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), 6, 0x2000);
}

// See https://flapenguin.me/elf-dt-hash
// See https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#hash
static void push_hash() {
    push_number(1, 4); // nbucket
    push_number(2, 4); // nchain, which is 2 because there is "<null>" and "a" in dynsym
    push_number(1, 4); // bucket[0] => 1, so dynsym[1] => "a"
    push_number(0, 4); // chain[0] is always 0
    push_number(0, 4); // chain[1] is 0, since if the symbol didn't match "a", there is no possible other match
    push_zeros(4); // Alignment
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
    push_section(0x1b, SHT_HASH, SHF_ALLOC, 0x120, 0x120, 20, 2, 0, 8, 4);

    // .dynsym: Dynamic linker symbol table section
    // 0x2160 to 0x21a0
    push_section(0x21, SHT_DYNSYM, SHF_ALLOC, 0x138, 0x138, 48, 3, 1, 8, 24);

    // .dynstr: String table section
    // 0x21a0 to 0x21e0
    push_section(0x29, SHT_STRTAB, SHF_ALLOC, 0x168, 0x168, 3, 0, 0, 1, 0);

    // .eh_frame: Program data section
    // 0x21e0 to 0x2220
    push_section(0x31, SHT_PROGBITS, SHF_ALLOC, 0x1000, 0x1000, 0, 0, 0, 8, 0);

    // .dynamic: Dynamic linking information section
    // 0x2220 to 0x2260
    push_section(0x3b, SHT_DYNAMIC, SHF_WRITE | SHF_ALLOC, 0x1f50, 0x1f50, 176, 3, 0, 8, 16);

    // .data: Data section
    // 0x2260 to 0x22a0
    push_section(0x44, SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, 0x2000, 0x2000, 3, 0, 0, 4, 0);

    // .symtab: Symbol table section
    // 0x22a0 to 0x22e0
    // The "link" of 8 is the section header index of the associated string table, so .strtab
    // The "info" of 4 is the symbol table index of the first non-local symbol, which is the 5th entry in push_symtab(), the global "a" symbol
    push_section(1, SHT_SYMTAB, 0, 0, 0x2008, 120, 8, 4, 8, 24);

    // .strtab: String table section
    // 0x22e0 to 0x2320
    push_section(0x09, SHT_PROGBITS | SHT_SYMTAB, 0, 0, 0x2080, 21, 0, 0, 1, 0);

    // .shstrtab: Section header string table section
    // 0x2320 to end
    push_section(0x11, SHT_PROGBITS | SHT_SYMTAB, 0, 0, 0x2095, 74, 0, 0, 1, 0);
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

static void generate_simple_so() {
    FILE *f = fopen("simple.so", "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // 0x0 to 0x40
    push_elf_header();

    // 0x40 to 0x120
    push_program_header(PT_LOAD, PF_R, 0, 0, 0, 0x1000, 0x1000, 0x1000);
    push_program_header(PT_LOAD, PF_R | PF_W, 0x1f50, 0x1f50, 0x1f50, 0xb3, 0xb3, 0x1000);
    push_program_header(PT_DYNAMIC, PF_R | PF_W, 0x1f50, 0x1f50, 0x1f50, 0xb0, 0xb0, 8);
    push_program_header(0x6474e552, PF_R, 0x1f50, 0x1f50, 0x1f50, 0xb0, 0xb0, 1);

    // 0x120 to 0x134
    push_hash();

    // 0x138 to 0x168
    push_dynsym();

    // 0x168 to 0x16d
    push_dynstr();

    // TODO: REMOVE!
    // 0x170 to 0x1f50
    push_zeros(0x1de0);

    // 0x1f50 to 0x2000
    push_dynamic();

    // 0x2000 to 0x2008
    push_data();

    // 0x2008 to 0x2080
    push_symtab();

    // 0x2080 to 0x2094
    push_strtab();

    // 0x2094 to 0x20e0
    push_shstrtab();

    // 0x20e0 to end
    push_section_headers();

    fwrite(bytes, sizeof(u8), bytes_size, f);

    fclose(f);
}

int main() {
    generate_simple_so();
}
