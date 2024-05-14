# Simple shared object generator

## Generating foo.so

`generate_simple_o.c` is simpler than `generate_simple_so.c`, but requires `ld` to turn the `.o` into an `.so`, so you can do either:

### With generate_simple_o.c + ld

Generate the object `foo.o` with `gcc generate_simple_o.c && ./a.out` (or with `nasm -f elf64 foo.s`), followed by `ld -shared --hash-style=sysv foo.o -o foo.so`.

### With generate_simple_so.c

Generate the shared library `foo.so` with `gcc generate_simple_so.c && ./a.out` (or with `nasm -f elf64 foo.s` followed by `ld -shared --hash-style=sysv foo.o -o foo.so`).

## Running foo.so

Compile and run main.c with `gcc run_so.c && ./a.out`, which loads `foo.so` dynamically and prints the `bar` text from it.
