# Simple shared object generator

## Running

`nasm -f elf64 foo.s`
`ld -shared foo.o -o foo.so`
`gcc main.c && ./a.out`
