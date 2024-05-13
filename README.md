# Simple shared object generator

## Generating foo.so

`generate_o.c` is simpler than `generate_so.c`, but requires `ld` to turn the `.o` into an `.so`, so you can do either:

### With generate_o.c + ld

Generate the object `foo.o` with `gcc generate_o.c && ./a.out` (or with `nasm -f elf64 foo.s`), followed by `ld -shared --hash-style=gnu foo.o -o foo.so`.

### With generate_so.c

Generate the shared library `foo.so` with `gcc generate_so.c && ./a.out` (or with `nasm -f elf64 foo.s` followed by `ld -shared --hash-style=gnu foo.o -o foo.so`).

## Running foo.so

Compile and run main.c with `gcc run_so.c && ./a.out`, which loads `foo.so` dynamically and prints the `bar` text from it.
