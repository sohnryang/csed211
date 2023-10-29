.section .text
.globl start
start:
mov $0x3e52dff5, %edi
mov $0x401879, %rax
callq *%rax
