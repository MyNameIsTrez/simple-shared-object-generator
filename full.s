; global define

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

; ; Declare an entity struct
; struc entity
; 	.a: resw 1 ; 2 bytes
; 	.b: resb 1 ; 1 byte
; endstruc

; ; Define a global entity called 'define'
; define:
; 	istruc entity
; 		at entity.a, dw 1337
; 		at entity.b, db 69
; 	iend

a: db "a^", 0
b: db "b^", 0
c: db "c^", 0
d: db "d^", 0
e: db "e^", 0
f: db "f^", 0
g: db "g^", 0
h: db "h^", 0

x: db "x^", 0
y: db "y^", 0

section .text

fn1_c:
	mov rax, 42
	ret

fn2_c:
	mov rax, 42
	ret
