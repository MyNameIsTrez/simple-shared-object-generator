gcc generate_complex_so.c && ./a.out && xxd foo.so > mine.hex

nasm -f elf64 foo.s && ld -shared --hash-style=sysv foo.o -o foo.so && xxd foo.so > goal.hex

nasm -f elf64 foo.s && ld -shared --hash-style=sysv foo.o -o foo.so && readelf -a foo.so

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

# With 4 symbols

nbuckets: 3 (what get_nbucket() returns when there are 4 symbols)
nchain: 5 (4 symbols + the SHT_UNDEF at index 0)

Bucket[ix] always has the value of the last entry that has `hash % nbucket` equal to `ix`

```
ix  bucket[ix]  name of first symbol in chain
--  ----------  -----------------------------
 0  2           c
 1  4           a
 2  1           b
```

Two asterisks ** and parens () indicate the start of a chain, so it's easier to see.

```
       SYMBOL TABLE    |
                       |
	name =             | hash =          hash %
ix  symtab[ix].st_name | elf_hash(name)  nbucket
--  ------------------ | --------------  -------
 0  <STN_UNDEF>        |
 1  b                  | 98              2 **    (0)
 2  c                  | 99              0 **    (0)
 3  d                  | 100             1        0 <-\
 4  a                  | 97              1 **    (3)--/
```

# With 8 symbols

nbuckets: 3 (what get_nbucket() returns when there are 8 symbols)
nchain: 9 (8 symbols + the SHT_UNDEF at index 0)

Bucket[ix] always has the value of the last entry that has `hash % nbucket` equal to `ix`

```
ix  bucket[ix]  name of first symbol in chain
--  ----------  -----------------------------
 0  4           c
 1  7           a
 2  8           e
```

Two asterisks ** and parens () indicate the start of a chain, so it's easier to see.

```
       SYMBOL TABLE    |
                       |
	name =             | hash =          hash %
ix  symtab[ix].st_name | elf_hash(name)  nbucket
--  ------------------ | --------------  -------
 0  <STN_UNDEF>        |
 1  b                  |  98             2          0 <---\
 2  f                  | 102             0          0 <-\ |
 3  g                  | 103             1      /-> 0   | |
 4  c                  |  99             0 **   |  (2)--/ |
 5  d                  | 100             1      \-- 3 <-\ |
 6  h                  | 104             2      /-> 1 --|-/
 7  a                  |  97             1 **   |  (5)--/
 8  e                  | 101             2 **   \--(6)
```

# With 16 symbols

nbuckets: 3 (what get_nbucket() returns when there are 16 symbols)
nchain: 17 (16 symbols + the SHT_UNDEF at index 0)

Bucket[ix] always has the value of the last entry that has `hash % nbucket` equal to `ix`

```
ix  bucket[ix]  name of first symbol in chain
--  ----------  -----------------------------
 0  11           c
 1  16           m
 2  15           e
```

Two asterisks ** and parens () indicate the start of a chain, so it's easier to see.

```
       SYMBOL TABLE    |
                       |
	name =             | hash =          hash %
ix  symtab[ix].st_name | elf_hash(name)  nbucket
--  ------------------ | --------------  -------
 0  <STN_UNDEF>        |
 1  b                  |  98             2     /---> 0
 2  p                  | 112             1     | /-> 0
 3  j                  | 106             1     | \-- 2 <---\
 4  n                  | 110             2     \---- 1 <---|-\
 5  f                  | 102             0           0 <-\ | |
 6  g                  | 103             1     /---> 3 --|-/ |
 7  o                  | 111             0     | /-> 5 --/   |
 8  l                  | 108             0     | \-- 7 <-\   |
 9  k                  | 107             2   /-|---> 4 --|---/
10  i                  | 105             0   | | /-> 8 --/
11  c                  |  99             0 **| | \-(10)
12  d                  | 100             1   | \---- 6 <-\
13  h                  | 104             2   \------ 9 <-|-\
14  a                  |  97             1      /-> 12 --/ |
15  e                  | 101             2 **   |  (13)----/
16  m                  | 109             1 **   \--(14)
```
