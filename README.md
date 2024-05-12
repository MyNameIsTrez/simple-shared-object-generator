# Simple shared object generator

## Running

1. Generate the shared library `foo.so` with `gcc generate_so.c && ./a.out` (or with `nasm -f elf64 foo.s` followed by `ld -shared foo.o -o foo.so`)
2. Compile and run main.c, which loads `foo.so` dynamically with `gcc run_so.c && ./a.out`, and you will see the text `bar` being printed
