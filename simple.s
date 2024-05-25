global a ; Exports the symbol "a"

section .data ; The default section is ".text", which is for code, not data

; Defines the symbol "a" containing the text "a^", with a null-terminator added
; (db stands for Define Byte)
a: db "a^", 0
