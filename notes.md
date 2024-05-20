gcc -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -fsanitize=address,undefined -g generate_full_so.c && ./a.out && xxd foo.so > mine.hex

nasm -f elf64 foo.s && ld -shared --hash-style=sysv foo.o -o foo.so && xxd foo.so > goal.hex

nasm -f elf64 foo.s && ld -shared --hash-style=sysv foo.o -o foo.so && readelf -a foo.so

nasm -f elf64 foo.s && ld -shared --hash-style=sysv foo.o -o foo.so && gcc run_so.c && ./a.out

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
