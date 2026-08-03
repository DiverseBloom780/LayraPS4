; sysv_abi_wrapper.asm
; ABI conversion: System V AMD64 (PS4 guest) -> Microsoft x64 (MSVC host)
;
; System V ABI:  arg1=RDI, arg2=RSI, arg3=RDX, arg4=RCX, arg5=R8, arg6=R9
; Microsoft x64: arg1=RCX, arg2=RDX, arg3=R8,  arg4=R9,  arg5=[RSP+32], arg6=[RSP+40]
;
; R11 = pointer to the actual MSVC host function (set by trampoline stub)
;
; We use R10 as scratch (volatile in both ABIs).

.code

sysv_to_msvc_thunk PROC
    ; On entry from guest SysV code:
    ;   RDI=arg1, RSI=arg2, RDX=arg3, RCX=arg4, R8=arg5, R9=arg6
    ;   R11=target MSVC function pointer
    ;   [RSP]=return address
    ;
    ; Save original R8/R9 (SysV args 5-6) in scratch before we overwrite R8.
    mov     r10, r8         ; r10 = SysV arg5 (save before R8 gets overwritten)
    mov     rax, r9         ; rax = SysV arg6 (save before any clobbering)
    
    ; Now shuffle SysV regs -> MSVC regs (order matters!):
    ; Do R8 and R9 first (from RDX and RCX), since we need old RDX/RCX values
    ; but we're about to overwrite RCX and RDX.
    mov     r8, rdx         ; MSVC arg3 = SysV arg3 (old RDX)
    mov     r9, rcx         ; MSVC arg4 = SysV arg4 (old RCX)
    ; Now RDX and RCX are free to overwrite
    mov     rcx, rdi        ; MSVC arg1 = SysV arg1
    mov     rdx, rsi        ; MSVC arg2 = SysV arg2
    
    ; Set up MSVC stack frame:
    ; We need 32 bytes shadow space + 16 bytes for args 5-6 + 8 bytes alignment
    ; [RSP] currently has return address (8 bytes, so RSP mod 16 = 8)
    ; sub 56 -> RSP mod 16 = 8+56 = 64 -> aligned to 16 before CALL
    sub     rsp, 56
    
    ; Place SysV args 5-6 on stack (MSVC args 5-6)
    mov     QWORD PTR [rsp+32], r10     ; MSVC arg5 = SysV arg5 (saved R8)
    mov     QWORD PTR [rsp+40], rax     ; MSVC arg6 = SysV arg6 (saved R9)
    
    ; Call the real MSVC function
    call    r11
    
    ; Clean up stack  
    add     rsp, 56
    
    ; RAX already contains the return value
    ret
    
sysv_to_msvc_thunk ENDP

END
