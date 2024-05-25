# Simple shared object generator

## generate_simple_o.c

1. `gcc generate_simple_o.c && ./a.out` (or `nasm -f elf64 simple.s`)
2. `ld -shared --hash-style=sysv simple.o -o simple.so`

### Verifying correctness

1. `gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_simple_o.c && ./a.out && xxd simple.o > mine.hex`
2. `nasm -f elf64 simple.s && xxd simple.o > goal.hex`
3. `diff mine.hex goal.hex` shows no output if the generated `simple.o` is correct

## generate_simple_so.c

1. `gcc generate_simple_so.c && ./a.out`

### Verifying correctness

1. `gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_simple_so.c && ./a.out && xxd simple.so > mine.hex`
2. `nasm -f elf64 simple.s && ld -shared --hash-style=sysv simple.o -o simple.so && xxd simple.so > goal.hex`
3. `diff mine.hex goal.hex` shows no output if the generated `simple.so` is correct







## Generating foo.so

`generate_simple_o.c` is simpler than `generate_simple_so.c`, but requires `ld` to turn the `.o` into an `.so`, so you can do either:

### With generate_simple_o.c + ld

Generate the object `foo.o` with `gcc generate_simple_o.c && ./a.out` , followed by `ld -shared --hash-style=sysv foo.o -o foo.so`.

### With generate_simple_so.c

Generate the shared library `foo.so` with `gcc generate_simple_so.c && ./a.out` (or with `nasm -f elf64 foo.s` followed by `ld -shared --hash-style=sysv foo.o -o foo.so`).

### With generate_full_so.c

`generate_full_so.c` is based on `generate_simple_so.c`, but isn't as incredibly hardcoded, since it can handle multiple labels.

Generate the shared library `foo.so` with `gcc generate_full_so.c && ./a.out`.

## Running foo.so

Compile and run main.c with `gcc run_so.c && ./a.out`, which loads `foo.so` dynamically and prints the `bar` text from it.
