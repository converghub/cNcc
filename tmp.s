.intel_syntax noprefix
.global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov rax, rbp
    sub rax, 16
    push rax
    push 0
    pop rdi
    pop rax
    push rax
    mov rax, rdi
    mov rdi, 8
    mul rdi
    mov rdi, rax
    pop rax
    add rax, rdi
    push rax
    push 1
    pop rdi
    pop rax
    mov [rax], rdi
    push rdi
    mov rax, rbp
    sub rax, 16
    push rax
    push 1
    pop rdi
    pop rax
    push rax
    mov rax, rdi
    mov rdi, 8
    mul rdi
    mov rdi, rax
    pop rax
    add rax, rdi
    push rax
    push 2
    pop rdi
    pop rax
    mov [rax], rdi
    push rdi
    mov rax, rbp
    sub rax, 16
    push rax
    push 0
    pop rdi
    pop rax
    push rax
    mov rax, rdi
    mov rdi, 8
    mul rdi
    mov rdi, rax
    pop rax
    add rax, rdi
    push rax
    pop rax
    mov rax, [rax]
    push rax
    mov rax, rbp
    sub rax, 16
    push rax
    push 1
    push 1
    pop rdi
    pop rax
    mul rdi
    push rax
    push 1
    pop rdi
    pop rax
    mul rdi
    push rax
    push 1
    push 1
    pop rdi
    pop rax
    mul rdi
    push rax
    pop rdi
    pop rax
    add rax, rdi
    push rax
    push 0
    pop rdi
    pop rax
    add rax, rdi
    push rax
    push 1
    pop rdi
    pop rax
    sub rax, rdi
    push rax
    pop rdi
    pop rax
    push rax
    mov rax, rdi
    mov rdi, 8
    mul rdi
    mov rdi, rax
    pop rax
    add rax, rdi
    push rax
    pop rax
    mov rax, [rax]
    push rax
    pop rdi
    pop rax
    add rax, rdi
    push rax
    jmp .main_end
.main_end:
    pop rax
    mov rsp, rbp
    pop rbp
    ret
