global define

global a
global b
global c
global d
global e
global f
global g
global h

global fn1_c
global fn2_c

section .data

define:
	dw 1337
	db 69

a: db "a^", 0
b: db "b^", 0
c: db "c^", 0
d: db "d^", 0
e: db "e^", 0
f: db "f^", 0
g: db "g^", 0
h: db "h^", 0

section .text

fn1_c:
	mov rax, 42
	ret

fn2_c:
	mov rax, 42
	ret
