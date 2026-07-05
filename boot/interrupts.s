%define USER_CS 0x1B
%define USER_DS 0x23

global enter_usermode
global enter_usermode_resume
global enter_usermode_return
global isr_syscall
global isr_gp_fault
global isr_page_fault
global idt_load

global isr_timer

extern timer_on_irq
extern syscall_handler
extern program_check_return
extern sched_should_preempt
extern program_save_context

section .bss
align 16
kernel_return_rsp:
    resq 1

section .text
idt_load:
    lidt [rdi]
    ret

enter_usermode:
    mov [rel kernel_return_rsp], rsp
    mov cr3, rdx
    push USER_DS
    push rsi
    push 0x202
    push USER_CS
    push rdi
    iretq

; rdi = pointer to user_ctx_t, rsi = pml4 physical
enter_usermode_resume:
    mov [rel kernel_return_rsp], rsp
    mov cr3, rsi

    ; Build the iret frame from the saved context.
    ; ctx offsets: r15=0 r14=8 r13=16 r12=24 r11=32 r10=40 r9=48 r8=56
    ;              rdi=64 rsi=72 rbp=80 rdx=88 rcx=96 rbx=104 rax=112
    ;              rip=120 rflags=128 rsp=136
    push USER_DS
    push qword [rdi + 136]    ; user rsp
    push qword [rdi + 128]    ; rflags
    push USER_CS
    push qword [rdi + 120]    ; user rip

    ; Restore GPRs. Restore rdi last via the stack.
    push qword [rdi + 64]     ; stash user rdi

    mov r15, [rdi + 0]
    mov r14, [rdi + 8]
    mov r13, [rdi + 16]
    mov r12, [rdi + 24]
    mov r11, [rdi + 32]
    mov r10, [rdi + 40]
    mov r9,  [rdi + 48]
    mov r8,  [rdi + 56]
    mov rsi, [rdi + 72]
    mov rbp, [rdi + 80]
    mov rdx, [rdi + 88]
    mov rcx, [rdi + 96]
    mov rbx, [rdi + 104]
    mov rax, [rdi + 112]

    pop rdi
    iretq

enter_usermode_return:
    mov rsp, [rel kernel_return_rsp]
    ret

isr_syscall:
    mov r10, rdi
    mov r11, rsi
    mov r12, rdx
    mov r13, rax

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push rbp
    push rbx

    mov rdi, r13
    mov rsi, r10
    mov rdx, r11
    mov rcx, r12
    call syscall_handler

    pop rbx
    pop rbp
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    call program_check_return
    iretq

isr_gp_fault:
    push rax
    extern idt_handle_gp_fault
    call idt_handle_gp_fault
    pop rax
    call program_check_return
    iretq

isr_page_fault:
    push rax
    extern idt_handle_page_fault
    call idt_handle_page_fault
    pop rax
    call program_check_return
    iretq

; Timer interrupt. Saves full context when preempting a user program.
; CPU-pushed iret frame (from user mode): ss, rsp, rflags, cs, rip
isr_timer:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Always advance the tick counter and run periodic work.
    call timer_on_irq

    ; Did the interrupt arrive while in user mode? Check saved CS.
    ; Stack layout now: r15(0) ... rax(112), rip(120), cs(128), rflags(136), rsp(144), ss(152)
    mov ax, [rsp + 128]
    cmp ax, USER_CS
    jne .from_kernel

    ; In user mode — ask the scheduler whether the quantum expired.
    call sched_should_preempt
    test eax, eax
    jz .restore_user

    ; Preempt: hand the saved register frame + iret frame to the kernel,
    ; which stores it into the running program's user_ctx_t.
    mov rdi, rsp
    call program_save_context

    ; Abandon the user-mode iret and return to the kernel caller
    ; (the instruction after enter_usermode / enter_usermode_resume).
    mov rsp, [rel kernel_return_rsp]
    ret

.restore_user:
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

.from_kernel:
    ; Timer fired in kernel mode — periodic tick only, then return.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq
