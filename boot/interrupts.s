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
extern kernel_cr3_phys

section .bss
align 16
kernel_return_rsp:
    resq 1

section .text
idt_load:
    lidt [rdi]
    ret

; User code clobbers every GPR, so the kernel's callee-saved registers
; must be stashed on the kernel stack before entering ring 3 and restored
; by enter_usermode_return (the C caller expects them preserved).
%macro SAVE_KERNEL_CALLEE_SAVED 0
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
%endmacro

enter_usermode:
    SAVE_KERNEL_CALLEE_SAVED
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
    SAVE_KERNEL_CALLEE_SAVED
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
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

; int 0x80 ABI: rax=number, rdi/rsi/rdx=args, rax=return.
; The user side only declares rcx/r11 clobbered, so every other GPR
; must survive the syscall — save the full set around the C handler.
isr_syscall:
    push rbx
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8                ; 14 pushes + 8 keeps the call 16-byte aligned

    ; NOTE: syscalls deliberately run on the USER page tables — handlers
    ; dereference user pointers (SYS_WRITE strings, copy_to_user targets),
    ; which only resolve under user CR3. The kernel data a syscall touches
    ; must therefore live below 4 MB (the supervisor identity map in the
    ; user tables). Periodic work is different: isr_timer switches to the
    ; kernel tables because its callees touch late-BSS globals.

    ; syscall_handler(number, arg1, arg2, arg3) — order avoids clobbering
    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    call syscall_handler

    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop rbx

    call program_check_return
    iretq

; Vectors 13/14 push a CPU error code — drop it before iretq.
; Handlers run on the kernel page tables (see isr_syscall).
isr_gp_fault:
    push rax
    mov rax, cr3
    push rax
    mov rax, [rel kernel_cr3_phys]
    mov cr3, rax
    extern idt_handle_gp_fault
    call idt_handle_gp_fault
    pop rax                   ; saved CR3
    mov cr3, rax
    pop rax
    add rsp, 8
    call program_check_return
    iretq

isr_page_fault:
    push rax
    mov rax, cr3
    push rax
    mov rax, [rel kernel_cr3_phys]
    mov cr3, rax
    extern idt_handle_page_fault
    call idt_handle_page_fault
    pop rax                   ; saved CR3
    mov cr3, rax
    pop rax
    add rsp, 8
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

    ; Run periodic work on the kernel page tables: the tick can arrive while
    ; user CR3 is active (ring 3, or ring 0 inside a syscall handler), and
    ; user tables only map the first 4 MB of kernel memory. A 16-byte slot
    ; keeps the calls 16-byte aligned.
    mov rbp, cr3
    sub rsp, 16
    mov [rsp], rbp
    mov rbp, [rel kernel_cr3_phys]
    mov cr3, rbp

    ; Always advance the tick counter and run periodic work.
    call timer_on_irq

    ; Did the interrupt arrive while in user mode? Check saved CS.
    ; Stack layout now: cr3(0), pad(8), r15(16) ... rax(128), rip(136),
    ; cs(144), rflags(152), rsp(160), ss(168)
    mov ax, [rsp + 144]
    cmp ax, USER_CS
    jne .from_kernel

    ; In user mode — ask the scheduler whether the quantum expired.
    call sched_should_preempt
    test eax, eax
    jz .restore_user

    ; Preempt: hand the saved register frame + iret frame to the kernel,
    ; which stores it into the running program's user_ctx_t. The kernel
    ; CR3 stays loaded — we are abandoning the user-mode iret.
    lea rdi, [rsp + 16]
    call program_save_context

    ; Abandon the user-mode iret and return to the kernel caller
    ; (the instruction after enter_usermode / enter_usermode_resume),
    ; restoring the callee-saved registers stashed on entry.
    mov rsp, [rel kernel_return_rsp]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

.restore_user:
    mov rbp, [rsp]            ; saved user CR3
    mov cr3, rbp
    add rsp, 16
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
    ; Timer fired in kernel mode — periodic tick only, then return with
    ; the interrupted context's CR3 (may be user CR3 inside a syscall).
    mov rbp, [rsp]
    mov cr3, rbp
    add rsp, 16
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
