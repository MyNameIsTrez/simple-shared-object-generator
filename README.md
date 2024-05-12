# Simple shared object generator

## Running

1. Compile `foo.o` from the foo.s nasm file with `nasm -f elf64 foo.s`
2. Generate `foo.so` shared library from `foo.o` with `ld -shared foo.o -o foo.so`
3. Compile and run main.c which loads `foo.so` dynamically with `gcc main.c && ./a.out`, and you will see the text `bar` being printed
