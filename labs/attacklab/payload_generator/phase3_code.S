.section .text
.globl start
start:
mov $0x55629f48, %rdi
mov $0x40194d, %rax
callq *%rax
