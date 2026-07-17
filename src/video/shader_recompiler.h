// src/video/shader_recompiler.h
// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Layra Shader Recompiler — converts raw GCN bytecode into an
// intermediate representation (LayraIR), then lowers LayraIR to
// SPIR-V for Vulkan consumption.

#pragma once

#include "common/types.h"
#include <string>
#include <vector>

namespace Video::Shader {

// ── GCN Instruction Encoding Categories ───────────────────────
// The PS4's Jaguar APU uses GCN 1.1/1.2 (Sea Islands / Volcanic Islands).
// Instructions are either 32-bit or 64-bit (with a literal constant appended).
enum class GcnEncoding : u8 {
  SOP2,   // Scalar ALU — two source operands
  SOPK,   // Scalar ALU — inline constant
  SOP1,   // Scalar ALU — one source operand
  SOPC,   // Scalar comparison
  SOPP,   // Scalar program flow (branch, barrier, etc.)
  VOP2,   // Vector ALU — two source operands
  VOP1,   // Vector ALU — one source operand
  VOPC,   // Vector comparison
  VOP3,   // Vector ALU — three operands (64-bit encoding)
  SMRD,   // Scalar memory read (GCN1) / SMEM (GCN3)
  MUBUF,  // Untyped buffer load/store
  MTBUF,  // Typed buffer load/store
  MIMG,   // Image (texture) operations
  DS,     // Local Data Share
  EXP,    // Export — writes VS output or PS colour
  FLAT,   // Flat memory (GCN3+)
  Unknown
};

// ── Decoded GCN Instruction ──────────────────────────────────
struct GcnInst {
  GcnEncoding encoding = GcnEncoding::Unknown;
  u32 opcode = 0;    // Opcode within the encoding group
  u32 raw = 0;       // First dword of the instruction
  u32 literal = 0;   // Literal constant (if 64-bit instruction)
  u8 length = 4;     // Instruction length in bytes (4 or 8)

  // Operand indices (meaning depends on encoding)
  u16 dst = 0;
  u16 src0 = 0;
  u16 src1 = 0;
  u16 src2 = 0;  // VOP3 only
};

// ── Layra Intermediate Representation (LayraIR) ──────────────
// A simple SSA-like representation of shader operations.
enum class IrOp : u8 {
  Nop,
  LoadConst,    // Load a scalar constant
  LoadInput,    // Load a VS input attribute or PS interpolant
  Add,          // dst = src0 + src1
  Mul,          // dst = src0 * src1
  Mad,          // dst = src0 * src1 + src2
  Mov,          // dst = src0
  Dot4,         // Four-component dot product
  Sample,       // Texture sample
  Export,       // Write to output (position or colour)
  Branch,       // Conditional branch
  Return,       // End of shader
};

struct IrInst {
  IrOp op = IrOp::Nop;
  u32 dst = 0;
  u32 src0 = 0;
  u32 src1 = 0;
  u32 src2 = 0;
  float imm_f = 0.0f;  // Immediate float
  u32 imm_u = 0;        // Immediate uint
};

// ── Recompiler Result ────────────────────────────────────────
enum class ShaderStage : u8 {
  Vertex,
  Pixel,
  Compute
};

struct RecompileResult {
  bool success = false;
  ShaderStage stage = ShaderStage::Vertex;
  std::vector<u32> spirv;       // SPIR-V binary words
  std::string error;
  u32 input_count = 0;          // Number of VS inputs
  u32 output_count = 0;         // Number of outputs (varyings or MRTs)
};

// ── Public API ───────────────────────────────────────────────

// Decode raw GCN bytecode into a list of instructions
std::vector<GcnInst> DecodeGcn(const u8 *bytecode, u32 sizeBytes);

// Convert decoded GCN into LayraIR
std::vector<IrInst> LiftToIR(const std::vector<GcnInst> &gcn,
                               ShaderStage stage);

// Lower LayraIR to SPIR-V binary
RecompileResult LowerToSpirv(const std::vector<IrInst> &ir,
                              ShaderStage stage);

// All-in-one: bytecode → SPIR-V
RecompileResult RecompileShader(const u8 *bytecode, u32 sizeBytes,
                                 ShaderStage stage);

} // namespace Video::Shader
