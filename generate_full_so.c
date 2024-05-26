#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BYTES 420420
#define MAX_SYMBOLS 420420

#define MAX_HASH_BUCKETS 32771 // From https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/elflink.c;h=6db6a9c0b4702c66d73edba87294e2a59ffafcf5;hb=refs/heads/master#l6560

// TODO: These need to be able to grow
#define CODE_OFFSET 0x1000
#define DYNAMIC_OFFSET 0x1f50
#define DATA_OFFSET 0x3000

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
    PF_X = 1, // Executable segment
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

static char *symbols[MAX_SYMBOLS];
static size_t symbols_size;

static bool is_substrs[MAX_SYMBOLS];

static size_t symbol_name_dynstr_offsets[MAX_SYMBOLS];

static u32 buckets[MAX_HASH_BUCKETS];

static u32 chains[MAX_SYMBOLS];
static size_t chains_size;

static char *shuffled_symbols[MAX_SYMBOLS];
static size_t shuffled_symbols_size;

static size_t shuffled_symbol_index_to_symbol_index[MAX_SYMBOLS];

static size_t data_offsets[MAX_SYMBOLS];
static size_t code_offsets[MAX_SYMBOLS];

static u8 bytes[MAX_BYTES];
static size_t bytes_size;

static size_t data_size;
static size_t hash_size;
static size_t dynsym_offset;
static size_t dynsym_size;
static size_t dynstr_offset;
static size_t dynstr_size;
static size_t symtab_offset;
static size_t symtab_size;
static size_t strtab_offset;
static size_t strtab_size;
static size_t shstrtab_offset;
static size_t shstrtab_size;
static size_t section_headers_offset;

static void overwrite_address(u64 n, size_t bytes_offset) {
    for (size_t i = 0; i < 8; i++) {
        // Little-endian requires the least significant byte first
        bytes[bytes_offset++] = n & 0xff;

        n >>= 8; // Shift right by one byte
    }
}

static void fix_elf_header_addresses() {
    // Section header table offset
    // 0x28 to 0x30
    overwrite_address(section_headers_offset, 0x28);
}

static void push_byte(u8 byte) {
    if (bytes_size + 1 > MAX_BYTES) {
        fprintf(stderr, "error: MAX_BYTES of %d was exceeded\n", MAX_BYTES);
        exit(EXIT_FAILURE);
    }

    bytes[bytes_size++] = byte;
}

static void push_zeros(size_t count) {
    for (size_t i = 0; i < count; i++) {
        push_byte(0);
    }
}

static void push_alignment(size_t alignment) {
    size_t excess = bytes_size % alignment;
    if (excess > 0) {
        push_zeros(alignment - excess);
    }
}

static void push_string(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        push_byte(str[i]);
    }
    push_byte('\0');
}

static void push_shstrtab(void) {
    shstrtab_offset = bytes_size;

    push_byte(0);
    push_string(".symtab");
    push_string(".strtab");
    push_string(".shstrtab");
    push_string(".hash");
    push_string(".dynsym");
    push_string(".dynstr");
    push_string(".eh_frame");
    push_string(".dynamic");
    push_string(".data");

    shstrtab_size = bytes_size - shstrtab_offset;

    push_alignment(8);
}

static void push_strtab(void) {
    strtab_offset = bytes_size;

    push_byte(0);
    push_string("full.s");
    
    // Global symbols
    // TODO: Add loop

    push_string("_DYNAMIC");

    // Local symbols
    // TODO: Don't loop through non-global symbols
    for (size_t i = 0; i < symbols_size; i++) {
        push_string(shuffled_symbols[i]);
    }

    strtab_size = bytes_size - strtab_offset;
}

static void push_number(u64 n, size_t byte_count) {
    while (n > 0) {
        // Little-endian requires the least significant byte first
        push_byte(n & 0xff);
        byte_count--;

        n >>= 8; // Shift right by one byte
    }

    // Optional padding
    push_zeros(byte_count);
}

// See https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-79797/index.html
// See https://docs.oracle.com/cd/E19683-01/816-1386/6m7qcoblj/index.html#chapter6-tbl-21
static void push_symbol_entry(u32 name, u16 info, u16 shndx, u32 offset) {
    push_number(name, 4); // Indexed into .strtab, because .symtab its "link" points to it
    push_number(info, 2);
    push_number(shndx, 2);
    push_number(offset, 4); // In executable and shared object files, st_value holds a virtual address

    // TODO: I'm confused by why we don't seem to need these
    // push_number(size, 4);
    // push_number(other, 4);

    push_zeros(24 - 12); // .symtab its entry_size is 24
}

static void push_symtab(void) {
    symtab_offset = bytes_size;

    // Null entry
    // TODO: ? to TODO: ?
    push_symbol_entry(0, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), SHN_UNDEF, 0);

    // "full.s" entry
    // TODO: ? to TODO: ?
    push_symbol_entry(1, ELF32_ST_INFO(STB_LOCAL, STT_FILE), SHN_ABS, 0);

    // TODO: ? entry
    // TODO: ? to TODO: ?
    push_symbol_entry(0, ELF32_ST_INFO(STB_LOCAL, STT_FILE), SHN_ABS, 0);

    // "_DYNAMIC" entry
    // TODO: ? to TODO: ?
    push_symbol_entry(10, ELF32_ST_INFO(STB_LOCAL, STT_OBJECT), 5, 0);

    u32 name = sizeof("full.s") + sizeof("_DYNAMIC");

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 3, 0);
    name += strlen("b") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 15, 0);
    name += strlen("f") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 18, 0);
    name += strlen("g") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 66, 0);
    name += strlen("fn1_c") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 66, 0);
    name += strlen("fn2_c") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 6, 0);
    name += strlen("c") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 9, 0);
    name += strlen("d") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 21, 0);
    name += strlen("h") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 0, 0);
    name += strlen("a") + 1;

    // TODO: ? to TODO: ?
    push_symbol_entry(name, ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), DATA_OFFSET + 12, 0);
    name += strlen("e") + 1;

    symtab_size = bytes_size - symtab_offset;
}

static void push_data(void) {
    // TODO: Use the data from the AST
    push_string("a^");
    push_string("b^");
    push_string("c^");
    push_string("d^");
    push_string("e^");
    push_string("f^");
    push_string("g^");
    push_string("h^");

    push_alignment(8);
}

// See https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-42444.html
static void push_dynamic_entry(u64 tag, u64 value) {
    push_number(tag, 8);
    push_number(value, 8);
}

static void push_dynamic() {
    push_dynamic_entry(DT_HASH, 0x120);
    push_dynamic_entry(DT_STRTAB, dynstr_offset);
    push_dynamic_entry(DT_SYMTAB, dynsym_offset);
    push_dynamic_entry(DT_STRSZ, dynstr_size);
    push_dynamic_entry(DT_SYMENT, 24);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
    push_dynamic_entry(DT_NULL, 0);
}

static void push_code(void) {
    // TODO: Use the code from the AST
    push_byte(0xb8);
    push_byte(0x2a);
    push_byte(0);
    push_byte(0);
    push_byte(0);
    push_byte(0xc3);
    push_byte(0xb8);
    push_byte(0x2a);
    push_byte(0);
    push_byte(0);
    push_byte(0);
    push_byte(0xc3);

    push_alignment(8);
}

static void push_dynstr(void) {
    dynstr_offset = bytes_size;

    // .dynstr always starts with a '\0'
    dynstr_size = 1;

    push_byte(0);
    for (size_t i = 0; i < symbols_size; i++) {
        if (!is_substrs[i]) {
            push_string(symbols[i]);
            dynstr_size += strlen(symbols[i]) + 1;
        }
    }

    push_alignment(8);
}

static u32 get_nbucket(void) {
    // From https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/elflink.c;h=6db6a9c0b4702c66d73edba87294e2a59ffafcf5;hb=refs/heads/master#l6560
    //
    // Array used to determine the number of hash table buckets to use
    // based on the number of symbols there are. If there are fewer than
    // 3 symbols we use 1 bucket, fewer than 17 symbols we use 3 buckets,
    // fewer than 37 we use 17 buckets, and so forth. We never use more
    // than MAX_HASH_BUCKETS (32771) buckets.
    static const u32 nbucket_options[] = {
        1, 3, 17, 37, 67, 97, 131, 197, 263, 521, 1031, 2053, 4099, 8209, 16411, MAX_HASH_BUCKETS, 0
    };

    u32 nbucket = 0;

    for (size_t i = 0; nbucket_options[i] != 0; i++) {
        nbucket = nbucket_options[i];

        if (symbols_size < nbucket_options[i + 1]) {
            break;
        }
    }

    return nbucket;
}

// From https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/elf.c#l193
static u32 elf_hash(const char *namearg) {
    u32 h = 0;

    for (const unsigned char *name = (const unsigned char *) namearg; *name; name++) {
        h = (h << 4) + *name;
        h ^= (h >> 24) & 0xf0;
    }

    return h & 0x0fffffff;
}

static void push_chain(u32 chain) {
    if (chains_size + 1 > MAX_SYMBOLS) {
        fprintf(stderr, "error: MAX_SYMBOLS of %d was exceeded\n", MAX_SYMBOLS);
        exit(EXIT_FAILURE);
    }

    chains[chains_size++] = chain;
}

// See https://flapenguin.me/elf-dt-hash
// See https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#hash
//
// Example with 16 symbols "abcdefghijklmnop":
// 
// nbuckets: 3 (what get_nbucket() returns when there are 16 symbols)
// nchain: 17 (16 symbols + the SHT_UNDEF at index 0)
// 
// Bucket[i] always has the value of the last entry that has `hash % nbucket` equal to `i`
// 
//  i  bucket[i]  name of first symbol in chain
// --  ---------  -----------------------------
//  0  11         c
//  1  16         m
//  2  15         e
// 
// Two asterisks ** and parens () indicate the start of a chain, so it's easier to see.
// 
//        SYMBOL TABLE   |
//                       |
// 	   name =            | hash =          bucket_index =
//  i  symtab[i].st_name | elf_hash(name)  hash % nbucket
// --  ----------------- | --------------  --------------  
//  0  <STN_UNDEF>       |
//  1  b                 |  98             2                 /---> 0
//  2  p                 | 112             1                 | /-> 0
//  3  j                 | 106             1                 | \-- 2 <---\.
//  4  n                 | 110             2                 \---- 1 <---|-\.
//  5  f                 | 102             0                       0 <-\ | |
//  6  g                 | 103             1                 /---> 3 --|-/ |
//  7  o                 | 111             0                 | /-> 5 --/   |
//  8  l                 | 108             0                 | \-- 7 <-\   |
//  9  k                 | 107             2               /-|---> 4 --|---/
// 10  i                 | 105             0               | | /-> 8 --/
// 11  c                 |  99             0 **            | | \-(10)
// 12  d                 | 100             1               | \---- 6 <-\.
// 13  h                 | 104             2               \------ 9 <-|-\.
// 14  a                 |  97             1                  /-> 12 --/ |
// 15  e                 | 101             2 **               |  (13)----/
// 16  m                 | 109             1 **               \--(14)
static void push_hash(void) {
    size_t start_size = bytes_size;

    u32 nbucket = get_nbucket();
    push_number(nbucket, 4);

    u32 nchain = 1 + symbols_size; // `1 + `, because index 0 is always STN_UNDEF (the value 0)
    push_number(nchain, 4);

    memset(buckets, 0, nbucket * sizeof(u32));

    chains_size = 0;

    push_chain(0); // The first entry in the chain is always STN_UNDEF

    for (size_t i = 0; i < symbols_size; i++) {
        u32 hash = elf_hash(shuffled_symbols[i]);
        u32 bucket_index = hash % nbucket;

        push_chain(buckets[bucket_index]);

        buckets[bucket_index] = i + 1;
    }

    for (size_t i = 0; i < nbucket; i++) {
        push_number(buckets[i], 4);
    }

    for (size_t i = 0; i < chains_size; i++) {
        push_number(chains[i], 4);
    }

    hash_size = bytes_size - start_size;

    push_alignment(8);
}

static void push_section_header(u32 name_offset, u32 type, u64 flags, u64 address, u64 offset, u64 size, u32 link, u32 info, u64 alignment, u64 entry_size) {
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

static void push_section_headers(void) {
    section_headers_offset = bytes_size;

    // Null section
    // TODO: ? to TODO: ?
    push_zeros(0x40);

    // .hash: Hash section
    // TODO: ? to TODO: ?
    push_section_header(0x1b, SHT_HASH, SHF_ALLOC, 0x120, 0x120, hash_size, 2, 0, 8, 4);

    // .dynsym: Dynamic linker symbol table section
    // TODO: ? to TODO: ?
    push_section_header(0x21, SHT_DYNSYM, SHF_ALLOC, dynsym_offset, dynsym_offset, dynsym_size, 3, 1, 8, 0x18);

    // .dynstr: String table section
    // TODO: ? to TODO: ?
    push_section_header(0x29, SHT_STRTAB, SHF_ALLOC, dynstr_offset, dynstr_offset, dynstr_size, 0, 0, 1, 0);

    // .eh_frame: Code section
    // TODO: ? to TODO: ?
    push_section_header(0x31, SHT_PROGBITS, SHF_ALLOC, CODE_OFFSET, CODE_OFFSET, 0, 0, 0, 8, 0);

    // .dynamic: Dynamic linking information section
    // TODO: ? to TODO: ?
    push_section_header(0x3b, SHT_DYNAMIC, SHF_WRITE | SHF_ALLOC, DYNAMIC_OFFSET, DYNAMIC_OFFSET, 0xb0, 3, 0, 8, 0x10);

    // .data: Data section
    // TODO: ? to TODO: ?
    push_section_header(0x44, SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, DATA_OFFSET, DATA_OFFSET, data_size, 0, 0, 4, 0);

    // .symtab: Symbol table section
    // TODO: ? to TODO: ?
    // The "link" of 8 is the section header index of the associated string table, so .strtab
    // The "info" of 4 is the symbol table index of the first non-local symbol, which is the 5th entry in push_symtab(), the global "g" symbol
    push_section_header(1, SHT_SYMTAB, 0, 0, symtab_offset, symtab_size, 8, 4, 8, 0x18);

    // .strtab: String table section
    // TODO: ? to TODO: ?
    push_section_header(0x09, SHT_PROGBITS | SHT_SYMTAB, 0, 0, strtab_offset, strtab_size, 0, 0, 1, 0);

    // .shstrtab: Section header string table section
    // TODO: ? to end
    push_section_header(0x11, SHT_PROGBITS | SHT_SYMTAB, 0, 0, shstrtab_offset, shstrtab_size, 0, 0, 1, 0);
}

static void push_dynsym(void) {
    dynsym_offset = bytes_size;

    // Null entry
    // 0x1d0 to 0x1e8
    push_symbol_entry(0, ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE), SHN_UNDEF, 0);

    // The symbols are pushed in shuffled_symbols order
    for (size_t i = 0; i < symbols_size; i++) {
        size_t symbol_index = shuffled_symbol_index_to_symbol_index[i];

        bool is_data = symbol_index < 8; // TODO: Use the data symbol count from the AST
        u16 shndx = is_data ? 7 : 4; // 7 is .symtab, 4 is .eh_frame
        u32 offset = is_data ? DATA_OFFSET + data_offsets[symbol_index] : CODE_OFFSET + code_offsets[symbol_index - 8]; // TODO: Use the data symbol count from the AST

        push_symbol_entry(symbol_name_dynstr_offsets[symbol_index], ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE), shndx, offset);
    }

    dynsym_size = bytes_size - dynsym_offset;
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

static void push_program_headers(void) {
    // 0x40 to 0x78
    push_program_header(PT_LOAD, PF_R, 0, 0, 0, 0x2f3, 0x2f3, 0x1000);

    // TODO: Use the data from the AST
    // TODO: `(symbols_size - 2)` is a temporary way to ignore the fn1_c and fn2_c labels
    // Note that it's possible to have data that isn't exported
    data_size = (symbols_size - 2) * sizeof("a^");

    // Code segment
    // 0x78 to 0xb0
    push_program_header(PT_LOAD, PF_R | PF_X, CODE_OFFSET, CODE_OFFSET, CODE_OFFSET, 12, 12, 0x1000);

    // TODO: ? segment
    // 0xb0 to 0xe8
    push_program_header(PT_LOAD, PF_R, 0x2000, 0x2000, 0x2000, 0, 0, 0x1000);

    // Program data
    // 0xe8 to 0x120
    push_program_header(PT_LOAD, PF_R | PF_W, 0x2f50, 0x2f50, 0x2f50, 0xb0 + data_size, 0xb0 + data_size, 0x1000);

    // TODO: ? segment
    // 0x120 to 0x158
    push_program_header(PT_DYNAMIC, PF_R | PF_W, 0x2f50, 0x2f50, 0x2f50, 0xb0, 0xb0, 8);

    // TODO: ? segment
    // 0x158 to 0x190
    push_program_header(0x6474e552, PF_R, 0x2f50, 0x2f50, 0x2f50, 0xb0, 0xb0, 1);
}

static void push_elf_header(void) {
    // Magic number
    // 0x0 to 0x4
    push_byte(0x7f);
    push_byte('E');
    push_byte('L');
    push_byte('F');

    // 64-bit
    // 0x4 to 0x5
    push_byte(2);

    // Little-endian
    // 0x5 to 0x6
    push_byte(1);

    // Version
    // 0x6 to 0x7
    push_byte(1);

    // SysV OS ABI
    // 0x7 to 0x8
    push_byte(0);

    // Padding
    // 0x8 to 0x10
    push_zeros(8);

    // Shared object
    // 0x10 to 0x12
    push_byte(ET_DYN);
    push_byte(0);

    // x86-64 instruction set architecture
    // 0x12 to 0x14
    push_byte(0x3E);
    push_byte(0);

    // Original version of ELF
    // 0x14 to 0x18
    push_byte(1);
    push_zeros(3);

    // Execution entry point address
    // 0x18 to 0x20
    push_zeros(8);

    // Program header table offset
    // 0x20 to 0x28
    push_byte(0x40);
    push_zeros(7);

    // Section header table offset (this value gets overwritten later)
    // 0x28 to 0x30
    push_zeros(8);

    // Processor-specific flags
    // 0x30 to 0x34
    push_zeros(4);

    // ELF header size
    // 0x34 to 0x36
    push_byte(0x40);
    push_byte(0);

    // Single program header size
    // 0x36 to 0x38
    push_byte(0x38);
    push_byte(0);

    // Number of program header entries
    // 0x38 to 0x3a
    push_byte(6);
    push_byte(0);

    // Single section header entry size
    // 0x3a to 0x3c
    push_byte(0x40);
    push_byte(0);

    // Number of section header entries
    // 0x3c to 0x3e
    push_byte(11);
    push_byte(0);

    // Index of entry with section names
    // 0x3e to 0x40
    push_byte(10);
    push_byte(0);
}

static void push_bytes() {
    // 0x0 to 0x40
    push_elf_header();

    // 0x40 to 0x190
    push_program_headers();

    // 0x190 to 0x1d0
    push_hash();

    // 0x1d0 to 0x2d8
    push_dynsym();

    // 0x2d8 to 0x2f8
    push_dynstr();

    // 0x2f8 to 0x1000
    push_zeros(CODE_OFFSET - bytes_size);

    // 0x1000 to 0x1010
    push_code();

    // 0x1010 to 0x1f50
    push_zeros(DYNAMIC_OFFSET - bytes_size);

    // 0x1f50 to TODO: ?
    push_dynamic();

    // TODO: ? to TODO: ?
    push_data();

    // TODO: ? to TODO: ?
    push_symtab();

    // TODO: ? to TODO: ?
    push_strtab();

    // TODO: ? to TODO: ?
    push_shstrtab();

    // TODO: ? to end
    push_section_headers();
}

static void init_code_offsets(void) {
    // TODO: Use the data from the AST
    for (size_t i = 0; i < 2; i++) {
        code_offsets[i] = i * 6; // fn1_c takes 6 bytes of instructions
    }
}

static void init_data_offsets(void) {
    // TODO: Use the data from the AST
    for (size_t i = 0; i < 8; i++) {
        data_offsets[i] = i * sizeof("a^");
    }
}

static void push_shuffled_symbol(char *shuffled_symbol) {
    if (shuffled_symbols_size + 1 > MAX_SYMBOLS) {
        fprintf(stderr, "error: MAX_SYMBOLS of %d was exceeded\n", MAX_SYMBOLS);
        exit(EXIT_FAILURE);
    }

    shuffled_symbols[shuffled_symbols_size++] = shuffled_symbol;
}

// This is solely here to put the symbols in the same weird order as ld does
// From https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/hash.c#l508
static unsigned long bfd_hash_hash(const char *string) {
    const unsigned char *s;
    unsigned long hash;
    unsigned int len;
    unsigned int c;

    hash = 0;
    len = 0;
    s = (const unsigned char *) string;
    while ((c = *s++) != '\0') {
        hash += c + (c << 17);
        hash ^= hash >> 2;
    }
    len = (s - (const unsigned char *) string) - 1;
    hash += len + (len << 17);
    hash ^= hash >> 2;
    return hash;
}

// See the documentation of push_hash() for how this function roughly works
//
// name | index
// "a"  | 3485
// "b"  |  245
// "c"  | 2224
// "d"  | 2763
// "e"  | 3574
// "f"  |  553
// "g"  |  872
// "h"  | 3042
// "i"  | 1868
// "j"  |  340
// "k"  | 1151
// "l"  | 1146
// "m"  | 3669
// "n"  |  429
// "o"  |  967
// "p"  |  256
//
// This gets shuffled by ld to this:
// (see https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/hash.c#l618)
// "b"
// "p"
// "j"
// "n"
// "f"
// "g"
// "o"
// "l"
// "k"
// "i"
// "c"
// "d"
// "h"
// "a"
// "e"
// "m"
static void generate_shuffled_symbols(void) {
    #define DEFAULT_SIZE 4051 // From https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/hash.c#l345

    static u32 buckets[DEFAULT_SIZE];

    memset(buckets, 0, DEFAULT_SIZE * sizeof(u32));

    chains_size = 0;

    push_chain(0); // The first entry in the chain is always STN_UNDEF

    for (size_t i = 0; i < symbols_size; i++) {
        u32 hash = bfd_hash_hash(symbols[i]);
        u32 bucket_index = hash % DEFAULT_SIZE;

        push_chain(buckets[bucket_index]);

        buckets[bucket_index] = i + 1;
    }

    for (size_t i = 0; i < DEFAULT_SIZE; i++) {
        u32 chain_index = buckets[i];
        if (chain_index == 0) {
            continue;
        }

        char *symbol = symbols[chain_index - 1];

        shuffled_symbol_index_to_symbol_index[shuffled_symbols_size] = chain_index - 1;

        push_shuffled_symbol(symbol);

        while (true) {
            chain_index = chains[chain_index];
            if (chain_index == 0) {
                break;
            }

            symbol = symbols[chain_index - 1];

            shuffled_symbol_index_to_symbol_index[shuffled_symbols_size] = chain_index - 1;

            push_shuffled_symbol(symbol);
        }
    }
}

// haystack="a" , needle="a" => returns 0
// haystack="ab", needle="b" => returns 1
// haystack="a" , needle="b" => returns -1
// haystack="a" , needle="ab" => returns -1
static size_t get_ending_index(char *haystack, char *needle) {
  // Go to the end of the haystack and the needle
  char *hp = haystack;
  while (*hp) {
    hp++;
  }
  char *np = needle;
  while (*np) {
    np++;
  }

  // If the needle is longer than the haystack, it can't fit
  if (np - needle > hp - haystack) {
    return -1;
  }

  while (true) {
    // If one of the characters doesn't match
    if (*hp != *np) {
      return -1;
    }

    // If the needle entirely fits into the end of the haystack,
    // return the index where needle starts in haystack
    if (np == needle) {
      return hp - haystack; 
    }

    hp--;
    np--;
  }
}

static void init_symbol_name_dynstr_offsets(void) {
    size_t offset = 1;

    static size_t parent_indices[MAX_SYMBOLS];
    static size_t substr_offsets[MAX_SYMBOLS];

    memset(parent_indices, -1, symbols_size * sizeof(size_t));

    memset(is_substrs, false, symbols_size * sizeof(bool));

    // This function could be optimized from O(n^2) to O(n) with a hash map
    for (size_t i = 0; i < symbols_size; i++) {
        char *symbol = symbols[i];

        size_t parent_index;
        size_t ending_index;
        for (parent_index = 0; parent_index < symbols_size; parent_index++) {
            if (i != parent_index) {
                ending_index = get_ending_index(symbols[parent_index], symbol);
                if (ending_index != (size_t)-1) {
                    break;
                }
            }
        }

        // If symbol wasn't in the end of another symbol
        bool is_substr = parent_index != symbols_size;

        if (is_substr) {
            parent_indices[i] = parent_index;
            substr_offsets[i] = ending_index;
        } else {
            symbol_name_dynstr_offsets[i] = offset;
            offset += strlen(symbol) + 1;
        }

        is_substrs[i] = is_substr;
    }

    // Now that all the parents have been given final offsets in .dynstr,
    // the symbols that are substrings of them know where they go inside them
    for (size_t i = 0; i < symbols_size; i++) {
        size_t parent_index = parent_indices[i];
        if (parent_index != (size_t)-1) {
            size_t parent_offset = symbol_name_dynstr_offsets[parent_index];
            symbol_name_dynstr_offsets[i] = parent_offset + substr_offsets[i];
        }
    }
}

static void push_symbol(char *symbol) {
    if (symbols_size + 1 > MAX_SYMBOLS) {
        fprintf(stderr, "error: MAX_SYMBOLS of %d was exceeded\n", MAX_SYMBOLS);
        exit(EXIT_FAILURE);
    }

    symbols[symbols_size++] = symbol;
}

static void reset(void) {
    symbols_size = 0;
    chains_size = 0;
    shuffled_symbols_size = 0;
    bytes_size = 0;
}

static void generate_simple_so(void) {
    reset();

    // TODO: Use the symbols from the AST
    push_symbol("a");
    push_symbol("b");
    push_symbol("c");
    push_symbol("d");
    push_symbol("e");
    push_symbol("f");
    push_symbol("g");
    push_symbol("h");
    push_symbol("fn1_c");
    push_symbol("fn2_c");

    init_symbol_name_dynstr_offsets();

    generate_shuffled_symbols();

    init_data_offsets();
    init_code_offsets();

    push_bytes();

    fix_elf_header_addresses();

    FILE *f = fopen("full.so", "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fwrite(bytes, sizeof(u8), bytes_size, f);
    fclose(f);
}

int main(void) {
    generate_simple_so();
}
