gcc generate_so.c && ./a.out && xxd foo.so > my_foo_so.hex

readelf -a foo.so
