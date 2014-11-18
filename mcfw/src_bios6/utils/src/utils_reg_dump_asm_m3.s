; Get reference to the program's normal entry point and the AMMU_init func.
;
          .cdecls C,NOLIST,"xdc/std.h","mcfw/interfaces/common_def/ti_vsys_common_def.h"
          .thumb

r0_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r0
r1_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r1
r2_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r2
r3_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r3
r4_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r4
r5_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r5
r6_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r6
r7_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r7
r8_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r8
r9_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r9
r10_offset:   .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r10
r11_offset:   .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r11
r12_offset:   .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.r12
sp_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.sp
lr_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.lr
pc_offset:    .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.pc
psr_offset:   .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.psr
ICSR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.ICSR
MMFSR_offset: .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.MMFSR
BFSR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.BFSR
UFSR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.UFSR
HFSR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.HFSR
DFSR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.DFSR
MMAR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.MMAR
BFAR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.BFAR
AFSR_offset:  .equ VSYS_SLAVE_CORE_EXCEPTION_CONTEXT_M3_S.AFSR


; Put these in symbol table so debugger can list them.
;
        .global utils_exception_asm_copy_regs

; This function is used to dump all register values into passed structure.
;
        .sect ".text:utils_exception_asm_copy_regs"     ; place this function in boot segment
        .clink

utils_exception_asm_copy_regs:
        .asmfunc
        STR   r0,[r0,#r0_offset]
        STR   r1,[r0,#r1_offset]
        STR   r2,[r0,#r2_offset]
        STR   r3,[r0,#r3_offset]
        STR   r4,[r0,#r4_offset]
        STR   r5,[r0,#r5_offset]
        STR   r6,[r0,#r6_offset]
        STR   r7,[r0,#r7_offset]
        STR   r8,[r0,#r8_offset]
        STR   r9,[r0,#r9_offset]
        STR   r10,[r0,#r10_offset]
        STR   r11,[r0,#r11_offset]
        STR   r12,[r0,#r12_offset]
        STR   sp,[r0,#sp_offset]
        STR   lr,[r0,#lr_offset]
        LDR   r1,utils_exception_asm_copy_regs_addr
        STR   r1,[r0,#pc_offset]
        BX    lr
        .endasmfunc

utils_exception_asm_copy_regs_addr .word utils_exception_asm_copy_regs
        .end


