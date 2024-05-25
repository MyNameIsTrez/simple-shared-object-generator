global a ; Exports the symbol "a"

section .data ; The default section is ".text", which is for code, not data

; Defines the symbol "a", containing the null-terminated data 'a', '^', '\0'
; (db stands for Define Byte)
a: db "a^", 0
