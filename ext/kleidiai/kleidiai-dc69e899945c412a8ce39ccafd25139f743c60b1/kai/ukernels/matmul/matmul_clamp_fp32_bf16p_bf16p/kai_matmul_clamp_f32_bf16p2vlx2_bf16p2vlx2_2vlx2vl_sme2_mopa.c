//
// SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
//
// SPDX-License-Identifier: Apache-2.0
//

// Do not flag up inline assembly blocks
#pragma GCC diagnostic ignored "-Woverlength-strings"

#if !defined(__aarch64__) || !defined(__ARM_FEATURE_SVE2)
#error This file must be compiled for AArch64, FEAT_SVE2.
#else  // Architectural features check.

#include "kai_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa.h"

#include <stddef.h>
#include <stdint.h>

#include "kai/kai_common.h"

static const size_t kai_mr = 2;
static const size_t kai_nr = 2;
static const size_t kai_kr = 2;
static const size_t kai_sr = 1;

size_t kai_get_m_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(void) {
    return kai_mr * kai_get_sme_vector_length_u32();
}

size_t kai_get_n_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(void) {
    return kai_nr * kai_get_sme_vector_length_u32();
}

size_t kai_get_mr_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(void) {
    return kai_mr * kai_get_sme_vector_length_u32();
}

size_t kai_get_nr_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(void) {
    return kai_nr * kai_get_sme_vector_length_u32();
}

size_t kai_get_kr_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(void) {
    return kai_kr;
}

size_t kai_get_sr_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(void) {
    return kai_sr;
}

size_t kai_get_lhs_packed_offset_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(size_t m_idx, size_t k) {
    KAI_ASSUME(m_idx % kai_get_m_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa() == 0);
    return m_idx * kai_roundup(k, kai_kr) * sizeof(uint16_t);
}

static size_t kai_get_rhs_packed_stride_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(size_t k) {
    return kai_get_n_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa() *
        (sizeof(float) + kai_roundup(k, kai_kr) * sizeof(uint16_t));
}

size_t kai_get_rhs_packed_offset_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(size_t n_idx, size_t k) {
    KAI_ASSUME(n_idx % kai_get_n_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa() == 0);
    const size_t block_idx = n_idx / kai_get_n_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa();
    return block_idx * kai_get_rhs_packed_stride_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(k);
}

size_t kai_get_dst_offset_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(
    size_t m_idx, size_t n_idx, size_t dst_stride) {
    KAI_ASSUME(m_idx % kai_get_m_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa() == 0);
    KAI_ASSUME(n_idx % kai_get_n_step_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa() == 0);

    return m_idx * dst_stride + n_idx * sizeof(float);
}

size_t kai_get_dst_size_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(size_t m, size_t n) {
    return m * n * sizeof(float);
}

void kai_run_matmul_clamp_f32_bf16p2vlx2_bf16p2vlx2_2vlx2vl_sme2_mopa(
    size_t m, size_t n, size_t k, const void* lhs_packed, const void* rhs_packed, void* dst, size_t dst_stride_row,
    size_t dst_stride_col, float clamp_min, float clamp_max) {
    KAI_ASSUME(dst_stride_col == sizeof(float));

    typedef struct {
        const void* A;
        const void* B;
        void* C;
        size_t ldcb;
        size_t M;
        size_t N;
        size_t K;
        float min;
        float max;
        void* accumulator_buffer;
        uint64_t flags;
    } KernelArgs;

    KernelArgs args;

    args.A = lhs_packed;
    args.B = rhs_packed;

    args.C = dst;
    args.ldcb = dst_stride_row;
    args.M = m;
    args.N = n;
    args.K = k;
    args.min = clamp_min;
    args.max = clamp_max;

    args.accumulator_buffer = NULL;
    args.flags = 0;

    __asm__ __volatile__(
        ".inst 0xd503477f  // SMSTART ZA\n"
        "ldr w14, [%x[args], %[offsetof_M]]\n"
        "mov x13, #0x0\n"
        "mov x11, #0x0\n"
        "ptrue p0.b\n"
        ".inst 0x25207811  // ptrue pn9.b\n"
        "ldr w10, [%x[args], %[offsetof_N]]\n"
        "ldr x9, [%x[args], %[offsetof_A]]\n"
        "1:"  // M loop
        "ldr x28, [%x[args], %[offsetof_B]]\n"
        "2:"  // N loop
        ".inst 0x25aa4570  // whilelt pn8.s, x11, x10, VLx2\n"
        "fmov z13.s, #1.0\n"
        ".inst 0xc00800ff  // zero { zad0, zad1, zad2, zad3, zad4, zad5, zad6, zad7 }\n"
        "mov x27, x9\n"
        ".inst 0xa040438e  // ld1w { z14.s-z15.s }, p8/Z, [x28]\n"  // Load bias
        "addvl x28, x28, #2\n"
        ".inst 0x808e01a0  // fmopa za0.s, p0/M, p0/M, z13.s, z14.s\n"
        ".inst 0x808f01a1  // fmopa za1.s, p0/M, p0/M, z13.s, z15.s\n"
        ".inst 0x808e01a2  // fmopa za2.s, p0/M, p0/M, z13.s, z14.s\n"
        ".inst 0x808f01a3  // fmopa za3.s, p0/M, p0/M, z13.s, z15.s\n"
        "ldr x20, [%x[args], %[offsetof_K]]\n"
        "add x20, x20, #0x1\n"
        "lsr x20, x20, #0x1\n"
        "lsr x21, x20, #0x2\n"
        "and x20, x20, #0x3\n"
        "cbz x21, 6f\n"
        "subs x21, x21, #0x1\n"
        ".inst 0xa1402772  // ld1h { z18.h, z26.h }, pn9.b/Z, [x27]\n"
        ".inst 0xa0402794  // ld1h { z20.h-z21.h }, pn9.b/Z, [x28]\n"
        ".inst 0xa1412764  // ld1h { z4.h, z12.h }, pn9.b/Z, [x27, #0x2, MUL VL]\n"
        ".inst 0xa041278a  // ld1h { z10.h-z11.h }, pn9.b/Z, [x28, #0x2, MUL VL]\n"
        ".inst 0xa1422773  // ld1h { z19.h, z27.h }, pn9.b/Z, [x27, #0x4, MUL VL]\n"
        ".inst 0xa0422798  // ld1h { z24.h-z25.h }, pn9.b/Z, [x28, #0x4, MUL VL]\n"
        ".inst 0xa043276e  // ld1h { z14.h-z15.h }, pn9.b/Z, [x27, #0x6, MUL VL]\n"
        "addvl x27, x27, #8\n"
        ".inst 0xa1432796  // ld1h { z22.h, z30.h }, pn9.b/Z, [x28, #0x6, MUL VL]\n"
        "addvl x28, x28, #8\n"
        "ble 5f\n"
        "4:"  // K loop
        ".inst 0x81940240  // bfmopa za0.s, p0/M, p0/M, z18.h, z20.h\n"
        "subs x21, x21, #0x1\n"
        ".inst 0x81950241  // bfmopa za1.s, p0/M, p0/M, z18.h, z21.h\n"
        ".inst 0x81940342  // bfmopa za2.s, p0/M, p0/M, z26.h, z20.h\n"
        ".inst 0x81950343  // bfmopa za3.s, p0/M, p0/M, z26.h, z21.h\n"
        ".inst 0xa1402772  // ld1h { z18.h, z26.h }, pn9.b/Z, [x27]\n"
        ".inst 0x818a0080  // bfmopa za0.s, p0/M, p0/M, z4.h, z10.h\n"
        ".inst 0xa0402794  // ld1h { z20.h-z21.h }, pn9.b/Z, [x28]\n"
        ".inst 0x818b0081  // bfmopa za1.s, p0/M, p0/M, z4.h, z11.h\n"
        ".inst 0x818a0182  // bfmopa za2.s, p0/M, p0/M, z12.h, z10.h\n"
        ".inst 0x818b0183  // bfmopa za3.s, p0/M, p0/M, z12.h, z11.h\n"
        ".inst 0xa1412764  // ld1h { z4.h, z12.h }, pn9.b/Z, [x27, #0x2, MUL VL]\n"
        ".inst 0x81980260  // bfmopa za0.s, p0/M, p0/M, z19.h, z24.h\n"
        ".inst 0xa041278a  // ld1h { z10.h-z11.h }, pn9.b/Z, [x28, #0x2, MUL VL]\n"
        ".inst 0x81990261  // bfmopa za1.s, p0/M, p0/M, z19.h, z25.h\n"
        ".inst 0x81980362  // bfmopa za2.s, p0/M, p0/M, z27.h, z24.h\n"
        ".inst 0x81990363  // bfmopa za3.s, p0/M, p0/M, z27.h, z25.h\n"
        ".inst 0xa1422773  // ld1h { z19.h, z27.h }, pn9.b/Z, [x27, #0x4, MUL VL]\n"
        ".inst 0xa0422798  // ld1h { z24.h-z25.h }, pn9.b/Z, [x28, #0x4, MUL VL]\n"
        ".inst 0x819601c0  // bfmopa za0.s, p0/M, p0/M, z14.h, z22.h\n"
        ".inst 0x819e01c1  // bfmopa za1.s, p0/M, p0/M, z14.h, z30.h\n"
        ".inst 0x819601e2  // bfmopa za2.s, p0/M, p0/M, z15.h, z22.h\n"
        ".inst 0x819e01e3  // bfmopa za3.s, p0/M, p0/M, z15.h, z30.h\n"
        ".inst 0xa043276e  // ld1h { z14.h-z15.h }, pn9.b/Z, [x27, #0x6, MUL VL]\n"
        "addvl x27, x27, #8\n"
        ".inst 0xa1432796  // ld1h { z22.h, z30.h }, pn9.b/Z, [x28, #0x6, MUL VL]\n"
        "addvl x28, x28, #8\n"
        "bgt 4b\n"
        "5:"  // K loop tail
        ".inst 0x81940240  // bfmopa za0.s, p0/M, p0/M, z18.h, z20.h\n"
        ".inst 0x81950241  // bfmopa za1.s, p0/M, p0/M, z18.h, z21.h\n"
        ".inst 0x81940342  // bfmopa za2.s, p0/M, p0/M, z26.h, z20.h\n"
        ".inst 0x81950343  // bfmopa za3.s, p0/M, p0/M, z26.h, z21.h\n"
        ".inst 0x818a0080  // bfmopa za0.s, p0/M, p0/M, z4.h, z10.h\n"
        ".inst 0x818b0081  // bfmopa za1.s, p0/M, p0/M, z4.h, z11.h\n"
        ".inst 0x818a0182  // bfmopa za2.s, p0/M, p0/M, z12.h, z10.h\n"
        ".inst 0x818b0183  // bfmopa za3.s, p0/M, p0/M, z12.h, z11.h\n"
        ".inst 0x81980260  // bfmopa za0.s, p0/M, p0/M, z19.h, z24.h\n"
        ".inst 0x81990261  // bfmopa za1.s, p0/M, p0/M, z19.h, z25.h\n"
        ".inst 0x81980362  // bfmopa za2.s, p0/M, p0/M, z27.h, z24.h\n"
        ".inst 0x81990363  // bfmopa za3.s, p0/M, p0/M, z27.h, z25.h\n"
        ".inst 0x819601c0  // bfmopa za0.s, p0/M, p0/M, z14.h, z22.h\n"
        ".inst 0x819e01c1  // bfmopa za1.s, p0/M, p0/M, z14.h, z30.h\n"
        ".inst 0x819601e2  // bfmopa za2.s, p0/M, p0/M, z15.h, z22.h\n"
        ".inst 0x819e01e3  // bfmopa za3.s, p0/M, p0/M, z15.h, z30.h\n"
        "6:"  // K oddments
        "cbz x20, 8f\n"
        "7:"  // K oddments: Loop
        ".inst 0xa040277c  // ld1h { z28.h-z29.h }, pn9.b/Z, [x27]\n"
        "subs x20, x20, #0x1\n"
        "addvl x27, x27, #2\n"
        ".inst 0xa1402787  // ld1h { z7.h, z15.h }, pn9.b/Z, [x28]\n"
        "addvl x28, x28, #2\n"
        ".inst 0x81870380  // bfmopa za0.s, p0/M, p0/M, z28.h, z7.h\n"
        ".inst 0x818f0381  // bfmopa za1.s, p0/M, p0/M, z28.h, z15.h\n"
        ".inst 0x818703a2  // bfmopa za2.s, p0/M, p0/M, z29.h, z7.h\n"
        ".inst 0x818f03a3  // bfmopa za3.s, p0/M, p0/M, z29.h, z15.h\n"
        "bgt 7b\n"
        "8:"  // K oddments: End
        "ldr x26, [%x[args], %[offsetof_C]]\n"
        "sub x25, x14, x13\n"
        "cntw x24\n"
        "ld1rw { z19.s }, p0/Z, [%x[args], %[offsetof_KernelArgs_min]]\n"
        "ldr x23, [%x[args], %[offsetof_ldcb]]\n"
        "cmp x25, x24\n"
        "ld1rw { z26.s }, p0/Z, [%x[args], %[offsetof_KernelArgs_max]]\n"
        "mov x12, #0x0\n"
        "csel x22, x25, x24, LT\n"
        "add x26, x26, x11, LSL #2\n"  // C += n
        "lsr x21, x22, #0x2\n"
        "madd x26, x13, x23, x26\n"  // C += m * ldc
        "and x20, x22, #0x3\n"
        "cbz x21, 11f\n"
        "10:"  // Store to output array: Accumulator row 0 loop
        ".inst 0xc0860404  // mova { z4.s-z7.s }, za0h.s[x12]\n"
        ".inst 0xc086042c  // mova { z12.s-z15.s }, za1h.s[x12]\n"
        ".inst 0xc1baca64  // fclamp { z4.s-z7.s }, z19.s, z26.s\n"
        ".inst 0xc1baca6c  // fclamp { z12.s-z15.s }, z19.s, z26.s\n"
        "add x12, x12, #0x4\n"
        "cmp x12, x21, LSL #2\n"
        ".inst 0xa1604344  // st1w { z4.s, z12.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        ".inst 0xa1604345  // st1w { z5.s, z13.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        ".inst 0xa1604346  // st1w { z6.s, z14.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        ".inst 0xa1604347  // st1w { z7.s, z15.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "blt 10b\n"
        "11:"  // Store to output array: Accumulator row 0 oddments
        "cbz x20, 12f\n"
        ".inst 0xc0860400  // mova { z0.s-z3.s }, za0h.s[x12]\n"
        ".inst 0xc0860428  // mova { z8.s-z11.s }, za1h.s[x12]\n"
        "subs x20, x20, #0x1\n"
        ".inst 0xc1baca60  // fclamp { z0.s-z3.s }, z19.s, z26.s\n"
        ".inst 0xc1baca68  // fclamp { z8.s-z11.s }, z19.s, z26.s\n"
        ".inst 0xa1604340  // st1w { z0.s, z8.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "beq 12f\n"
        "subs x20, x20, #0x1\n"
        ".inst 0xa1604341  // st1w { z1.s, z9.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "beq 12f\n"
        ".inst 0xa1604342  // st1w { z2.s, z10.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "12:"  // Store to output array: Accumulator row 0 oddments: End
        "subs x25, x25, x22\n"
        "beq 16f\n"
        "cmp x25, x24\n"
        "mov x12, #0x0\n"
        "csel x20, x25, x24, LT\n"
        "lsr x21, x20, #0x2\n"
        "and x20, x20, #0x3\n"
        "cbz x21, 14f\n"
        "13:"  // Store to output array: Accumulator row 1 loop
        ".inst 0xc0860454  // mova { z20.s-z23.s }, za2h.s[x12]\n"
        ".inst 0xc086047c  // mova { z28.s-z31.s }, za3h.s[x12]\n"
        ".inst 0xc1baca74  // fclamp { z20.s-z23.s }, z19.s, z26.s\n"
        ".inst 0xc1baca7c  // fclamp { z28.s-z31.s }, z19.s, z26.s\n"
        "add x12, x12, #0x4\n"
        "cmp x12, x21, LSL #2\n"
        ".inst 0xa1604354  // st1w { z20.s, z28.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        ".inst 0xa1604355  // st1w { z21.s, z29.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        ".inst 0xa1604356  // st1w { z22.s, z30.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        ".inst 0xa1604357  // st1w { z23.s, z31.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "blt 13b\n"
        "14:"  // Store to output array: Accumulator row 1 oddments
        "cbz x20, 15f\n"
        ".inst 0xc0860444  // mova { z4.s-z7.s }, za2h.s[x12]\n"
        ".inst 0xc086046c  // mova { z12.s-z15.s }, za3h.s[x12]\n"
        "subs x20, x20, #0x1\n"
        ".inst 0xc1baca64  // fclamp { z4.s-z7.s }, z19.s, z26.s\n"
        ".inst 0xc1baca6c  // fclamp { z12.s-z15.s }, z19.s, z26.s\n"
        ".inst 0xa1604344  // st1w { z4.s, z12.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "beq 15f\n"
        "subs x20, x20, #0x1\n"
        ".inst 0xa1604345  // st1w { z5.s, z13.s }, p8, [x26]\n"
        "add x26, x26, x23\n"
        "beq 15f\n"
        ".inst 0xa1604346  // st1w { z6.s, z14.s }, p8, [x26]\n"
        "15:"  // Store to output array: Accumulator row 1 oddments: End
        "16:"  // Store to output array: End
        "incw x11, ALL, MUL #2\n"
        "cmp x11, x10\n"
        "blt 2b\n"
        "incw x13, ALL, MUL #2\n"
        "mov x11, #0x0\n"
        "cmp x13, x14\n"
        "mov x9, x27\n"
        "blt 1b\n"
        ".inst 0xd503467f  // SMSTOP\n"
        :
        : [args] "r"(&args), [offsetof_A] "I"(offsetof(KernelArgs, A)), [offsetof_B] "I"(offsetof(KernelArgs, B)),
          [offsetof_C] "I"(offsetof(KernelArgs, C)), [offsetof_K] "I"(offsetof(KernelArgs, K)),
          [offsetof_KernelArgs_max] "I"(offsetof(KernelArgs, max)),
          [offsetof_KernelArgs_min] "I"(offsetof(KernelArgs, min)), [offsetof_M] "I"(offsetof(KernelArgs, M)),
          [offsetof_N] "I"(offsetof(KernelArgs, N)), [offsetof_ldcb] "I"(offsetof(KernelArgs, ldcb))
        : "cc", "memory", "p0", "p1", "p10", "p11", "p12", "p13", "p14", "p15", "p2", "p3", "p4", "p5", "p6", "p7",
          "p8", "p9", "x10", "x11", "x12", "x13", "x14", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28",
          "x9", "z0", "z1", "z10", "z11", "z12", "z13", "z14", "z15", "z16", "z17", "z18", "z19", "z2", "z20", "z21",
          "z22", "z23", "z24", "z25", "z26", "z27", "z28", "z29", "z3", "z30", "z31", "z4", "z5", "z6", "z7", "z8",
          "z9");
}

#endif  // Architectural features check.
