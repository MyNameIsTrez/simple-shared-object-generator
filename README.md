# Simple shared object generator

## generate_simple_o.c

1. `gcc generate_simple_o.c && ./a.out` (or `nasm -f elf64 simple.s`)
2. `ld -shared --hash-style=sysv simple.o -o simple.so`
3. `gcc run_simple.c && ./a.out`, which should print `a^`, coming from simple.o

### Verifying correctness

1. `gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_simple_o.c && ./a.out && xxd simple.o > mine.hex`
2. `nasm -f elf64 simple.s && xxd simple.o > goal.hex`
3. `diff mine.hex goal.hex` shows no output if the generated `simple.o` is correct

## generate_simple_so.c

1. `gcc generate_simple_so.c && ./a.out`
2. `gcc run_simple.c && ./a.out`, which should print `a^`, coming from simple.so

### Verifying correctness

1. `gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_simple_so.c && ./a.out && xxd simple.so > mine.hex`
2. `nasm -f elf64 simple.s && ld -shared --hash-style=sysv simple.o -o simple.so && xxd simple.so > goal.hex`
3. `diff mine.hex goal.hex` shows no output if the generated `simple.so` is correct

## generate_full_so.c

1. `gcc generate_full_so.c && ./a.out`
2. `gcc run_full.c && ./a.out`, which should print `a^` and `42`, coming from full.so

### Verifying correctness

1. `gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_full_so.c && ./a.out && xxd full.so > mine.hex`
2. `nasm -f elf64 full.s && ld -shared --hash-style=sysv full.o -o full.so && xxd full.so > goal.hex`
3. `diff mine.hex goal.hex` shows no output if the generated `full.so` is correct
