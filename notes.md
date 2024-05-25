gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_simple_o.c && ./a.out && xxd simple.o > mine.hex

nasm -f elf64 simple.s && xxd simple.o > goal.hex





gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g generate_full_so.c && ./a.out && xxd full.so > mine.hex

nasm -f elf64 full.s && ld -shared --hash-style=sysv full.o -o full.so && xxd full.so > goal.hex

nasm -f elf64 full.s && ld -shared --hash-style=sysv full.o -o full.so && readelf -a full.so

nasm -f elf64 full.s && ld -shared --hash-style=sysv full.o -o full.so && gcc run_so.c && ./a.out

```
| line | nbucket | symbol name |
| 1    | 1       | a           |
| 3    | 3       | c           |
| 17   | 11      | q           |
| 37   | 25      | k_          |
| 67   | 43      | o__         |
| 97   | 61      | s___        |
| 131  | 83      | a_____      |
```
