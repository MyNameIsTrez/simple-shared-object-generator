gcc generate_simple_so.c && ./a.out && xxd foo.so > my_foo_so.hex

nasm -f elf64 foo.s && ld -shared --hash-style=sysv foo.o -o foo.so && readelf -a foo.so
