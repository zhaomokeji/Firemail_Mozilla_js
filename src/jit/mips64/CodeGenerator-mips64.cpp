/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "jit/mips64/CodeGenerator-mips64.h"

#include "mozilla/MathAlgorithms.h"

#include "jit/CodeGenerator.h"
#include "jit/JitFrames.h"
#include "jit/JitRealm.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"
#include "js/Conversions.h"
#include "vm/Shape.h"
#include "vm/TraceLogging.h"

#include "jit/MacroAssembler-inl.h"
#include "jit/shared/CodeGenerator-shared-inl.h"

using namespace js;
using namespace js::jit;

ValueOperand CodeGeneratorMIPS64::ToValue(LInstruction* ins, size_t pos) {
  return ValueOperand(ToRegister(ins->getOperand(pos)));
}

ValueOperand CodeGeneratorMIPS64::ToTempValue(LInstruction* ins, size_t pos) {
  return ValueOperand(ToRegister(ins->getTemp(pos)));
}

void CodeGenerator::visitBox(LBox* box) {
  const LAllocation* in = box->getOperand(0);
  ValueOperand result = ToOutValue(box);

  masm.moveValue(TypedOrValueRegister(box->type(), ToAnyRegister(in)), result);
}

void CodeGenerator::visitUnbox(LUnbox* unbox) {
  MUnbox* mir = unbox->mir();

  Register result = ToRegister(unbox->output());

  if (mir->fallible()) {
    const ValueOperand value = ToValue(unbox, LUnbox::Input);
    Label bail;
    switch (mir->type()) {
      case MIRType::Int32:
        masm.fallibleUnboxInt32(value, result, &bail);
        break;
      case MIRType::Boolean:
        masm.fallibleUnboxBoolean(value, result, &bail);
        break;
      case MIRType::Object:
        masm.fallibleUnboxObject(value, result, &bail);
        break;
      case MIRType::String:
        masm.fallibleUnboxString(value, result, &bail);
        break;
      case MIRType::Symbol:
        masm.fallibleUnboxSymbol(value, result, &bail);
        break;
      case MIRType::BigInt:
        masm.fallibleUnboxBigInt(value, result, &bail);
        break;
      default:
        MOZ_CRASH("Given MIRType cannot be unboxed.");
    }
    bailoutFrom(&bail, unbox->snapshot());
    return;
  }

  LAllocation* input = unbox->getOperand(LUnbox::Input);
  if (input->isRegister()) {
    Register inputReg = ToRegister(input);
    switch (mir->type()) {
      case MIRType::Int32:
        masm.unboxInt32(inputReg, result);
        break;
      case MIRType::Boolean:
        masm.unboxBoolean(inputReg, result);
        break;
      case MIRType::Object:
        masm.unboxObject(inputReg, result);
        break;
      case MIRType::String:
        masm.unboxString(inputReg, result);
        break;
      case MIRType::Symbol:
        masm.unboxSymbol(inputReg, result);
        break;
      case MIRType::BigInt:
        masm.unboxBigInt(inputReg, result);
        break;
      default:
        MOZ_CRASH("Given MIRType cannot be unboxed.");
    }
    return;
  }

  Address inputAddr = ToAddress(input);
  switch (mir->type()) {
    case MIRType::Int32:
      masm.unboxInt32(inputAddr, result);
      break;
    case MIRType::Boolean:
      masm.unboxBoolean(inputAddr, result);
      break;
    case MIRType::Object:
      masm.unboxObject(inputAddr, result);
      break;
    case MIRType::String:
      masm.unboxString(inputAddr, result);
      break;
    case MIRType::Symbol:
      masm.unboxSymbol(inputAddr, result);
      break;
    case MIRType::BigInt:
      masm.unboxBigInt(inputAddr, result);
      break;
    default:
      MOZ_CRASH("Given MIRType cannot be unboxed.");
  }
}

void CodeGeneratorMIPS64::splitTagForTest(const ValueOperand& value,
                                          ScratchTagScope& tag) {
  masm.splitTag(value.valueReg(), tag);
}

void CodeGenerator::visitCompareB(LCompareB* lir) {
  MCompare* mir = lir->mir();

  const ValueOperand lhs = ToValue(lir, LCompareB::Lhs);
  const LAllocation* rhs = lir->rhs();
  const Register output = ToRegister(lir->output());

  MOZ_ASSERT(mir->jsop() == JSOp::StrictEq || mir->jsop() == JSOp::StrictNe);
  Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());

  // Load boxed boolean in ScratchRegister.
  if (rhs->isConstant()) {
    masm.moveValue(rhs->toConstant()->toJSValue(),
                   ValueOperand(ScratchRegister));
  } else {
    masm.boxValue(JSVAL_TYPE_BOOLEAN, ToRegister(rhs), ScratchRegister);
  }

  // Perform the comparison.
  masm.cmpPtrSet(cond, lhs.valueReg(), ScratchRegister, output);
}

void CodeGenerator::visitCompareBAndBranch(LCompareBAndBranch* lir) {
  MCompare* mir = lir->cmpMir();
  const ValueOperand lhs = ToValue(lir, LCompareBAndBranch::Lhs);
  const LAllocation* rhs = lir->rhs();

  MOZ_ASSERT(mir->jsop() == JSOp::StrictEq || mir->jsop() == JSOp::StrictNe);

  // Load boxed boolean in ScratchRegister.
  if (rhs->isConstant()) {
    masm.moveValue(rhs->toConstant()->toJSValue(),
                   ValueOperand(ScratchRegister));
  } else {
    masm.boxValue(JSVAL_TYPE_BOOLEAN, ToRegister(rhs), ScratchRegister);
  }

  // Perform the comparison.
  Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
  emitBranch(lhs.valueReg(), ScratchRegister, cond, lir->ifTrue(),
             lir->ifFalse());
}

void CodeGenerator::visitCompareBitwise(LCompareBitwise* lir) {
  MCompare* mir = lir->mir();
  Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
  const ValueOperand lhs = ToValue(lir, LCompareBitwise::LhsInput);
  const ValueOperand rhs = ToValue(lir, LCompareBitwise::RhsInput);
  const Register output = ToRegister(lir->output());

  MOZ_ASSERT(IsEqualityOp(mir->jsop()));

  masm.cmpPtrSet(cond, lhs.valueReg(), rhs.valueReg(), output);
}

void CodeGenerator::visitCompareBitwiseAndBranch(
    LCompareBitwiseAndBranch* lir) {
  MCompare* mir = lir->cmpMir();
  Assembler::Condition cond = JSOpToCondition(mir->compareType(), mir->jsop());
  const ValueOperand lhs = ToValue(lir, LCompareBitwiseAndBranch::LhsInput);
  const ValueOperand rhs = ToValue(lir, LCompareBitwiseAndBranch::RhsInput);

  MOZ_ASSERT(mir->jsop() == JSOp::Eq || mir->jsop() == JSOp::StrictEq ||
             mir->jsop() == JSOp::Ne || mir->jsop() == JSOp::StrictNe);

  emitBranch(lhs.valueReg(), rhs.valueReg(), cond, lir->ifTrue(),
             lir->ifFalse());
}

void CodeGenerator::visitCompareI64(LCompareI64* lir) {
  MCompare* mir = lir->mir();
  MOZ_ASSERT(mir->compareType() == MCompare::Compare_Int64 ||
             mir->compareType() == MCompare::Compare_UInt64);

  const LInt64Allocation lhs = lir->getInt64Operand(LCompareI64::Lhs);
  const LInt64Allocation rhs = lir->getInt64Operand(LCompareI64::Rhs);
  Register lhsReg = ToRegister64(lhs).reg;
  Register output = ToRegister(lir->output());
  Register rhsReg;
  ScratchRegisterScope scratch(masm);

  if (IsConstant(rhs)) {
    rhsReg = scratch;
    masm.ma_li(rhsReg, ImmWord(ToInt64(rhs)));
  } else if (rhs.value().isGeneralReg()) {
    rhsReg = ToRegister64(rhs).reg;
  } else {
    rhsReg = scratch;
    masm.loadPtr(ToAddress(rhs.value()), rhsReg);
  }

  bool isSigned = mir->compareType() == MCompare::Compare_Int64;
  masm.cmpPtrSet(JSOpToCondition(lir->jsop(), isSigned), lhsReg, rhsReg,
                 output);
}

void CodeGenerator::visitCompareI64AndBranch(LCompareI64AndBranch* lir) {
  MCompare* mir = lir->cmpMir();
  MOZ_ASSERT(mir->compareType() == MCompare::Compare_Int64 ||
             mir->compareType() == MCompare::Compare_UInt64);

  const LInt64Allocation lhs = lir->getInt64Operand(LCompareI64::Lhs);
  const LInt64Allocation rhs = lir->getInt64Operand(LCompareI64::Rhs);
  Register lhsReg = ToRegister64(lhs).reg;
  Register rhsReg;
  ScratchRegisterScope scratch(masm);

  if (IsConstant(rhs)) {
    rhsReg = scratch;
    masm.ma_li(rhsReg, ImmWord(ToInt64(rhs)));
  } else if (rhs.value().isGeneralReg()) {
    rhsReg = ToRegister64(rhs).reg;
  } else {
    rhsReg = scratch;
    masm.loadPtr(ToAddress(rhs.value()), rhsReg);
  }

  bool isSigned = mir->compareType() == MCompare::Compare_Int64;
  Assembler::Condition cond = JSOpToCondition(lir->jsop(), isSigned);
  emitBranch(lhsReg, rhsReg, cond, lir->ifTrue(), lir->ifFalse());
}

void CodeGenerator::visitDivOrModI64(LDivOrModI64* lir) {
  Register lhs = ToRegister(lir->lhs());
  Register rhs = ToRegister(lir->rhs());
  Register output = ToRegister(lir->output());

  Label done;

  // Handle divide by zero.
  if (lir->canBeDivideByZero()) {
    Label nonZero;
    masm.ma_b(rhs, rhs, &nonZero, Assembler::NonZero);
    masm.wasmTrap(wasm::Trap::IntegerDivideByZero, lir->bytecodeOffset());
    masm.bind(&nonZero);
  }

  // Handle an integer overflow exception from INT64_MIN / -1.
  if (lir->canBeNegativeOverflow()) {
    Label notOverflow;
    masm.branchPtr(Assembler::NotEqual, lhs, ImmWord(INT64_MIN), &notOverflow);
    masm.branchPtr(Assembler::NotEqual, rhs, ImmWord(-1), &notOverflow);
    if (lir->mir()->isMod()) {
      masm.ma_xor(output, output);
    } else {
      masm.wasmTrap(wasm::Trap::IntegerOverflow, lir->bytecodeOffset());
    }
    masm.jump(&done);
    masm.bind(&notOverflow);
  }

#ifdef MIPSR6
  if (lir->mir()->isMod()) {
    masm.as_dmod(output, lhs, rhs);
  } else {
    masm.as_ddiv(output, lhs, rhs);
  }
#else
  masm.as_ddiv(lhs, rhs);
  if (lir->mir()->isMod()) {
    masm.as_mfhi(output);
  } else {
    masm.as_mflo(output);
  }
#endif
  masm.bind(&done);
}

void CodeGenerator::visitUDivOrModI64(LUDivOrModI64* lir) {
  Register lhs = ToRegister(lir->lhs());
  Register rhs = ToRegister(lir->rhs());
  Register output = ToRegister(lir->output());

  Label done;

  // Prevent divide by zero.
  if (lir->canBeDivideByZero()) {
    Label nonZero;
    masm.ma_b(rhs, rhs, &nonZero, Assembler::NonZero);
    masm.wasmTrap(wasm::Trap::IntegerDivideByZero, lir->bytecodeOffset());
    masm.bind(&nonZero);
  }

#ifdef MIPSR6
  if (lir->mir()->isMod()) {
    masm.as_dmodu(output, lhs, rhs);
  } else {
    masm.as_ddivu(output, lhs, rhs);
  }
#else
  masm.as_ddivu(lhs, rhs);
  if (lir->mir()->isMod()) {
    masm.as_mfhi(output);
  } else {
    masm.as_mflo(output);
  }
#endif
  masm.bind(&done);
}

template <typename T>
void CodeGeneratorMIPS64::emitWasmLoadI64(T* lir) {
  const MWasmLoad* mir = lir->mir();

  Register ptrScratch = InvalidReg;
  if (!lir->ptrCopy()->isBogusTemp()) {
    ptrScratch = ToRegister(lir->ptrCopy());
  }

  if (IsUnaligned(mir->access())) {
    masm.wasmUnalignedLoadI64(mir->access(), HeapReg, ToRegister(lir->ptr()),
                              ptrScratch, ToOutRegister64(lir),
                              ToRegister(lir->getTemp(1)));
  } else {
    masm.wasmLoadI64(mir->access(), HeapReg, ToRegister(lir->ptr()), ptrScratch,
                     ToOutRegister64(lir));
  }
}

void CodeGenerator::visitWasmLoadI64(LWasmLoadI64* lir) {
  emitWasmLoadI64(lir);
}

void CodeGenerator::visitWasmUnalignedLoadI64(LWasmUnalignedLoadI64* lir) {
  emitWasmLoadI64(lir);
}

template <typename T>
void CodeGeneratorMIPS64::emitWasmStoreI64(T* lir) {
  const MWasmStore* mir = lir->mir();

  Register ptrScratch = InvalidReg;
  if (!lir->ptrCopy()->isBogusTemp()) {
    ptrScratch = ToRegister(lir->ptrCopy());
  }

  if (IsUnaligned(mir->access())) {
    masm.wasmUnalignedStoreI64(mir->access(), ToRegister64(lir->value()),
                               HeapReg, ToRegister(lir->ptr()), ptrScratch,
                               ToRegister(lir->getTemp(1)));
  } else {
    masm.wasmStoreI64(mir->access(), ToRegister64(lir->value()), HeapReg,
                      ToRegister(lir->ptr()), ptrScratch);
  }
}

void CodeGenerator::visitWasmStoreI64(LWasmStoreI64* lir) {
  emitWasmStoreI64(lir);
}

void CodeGenerator::visitWasmUnalignedStoreI64(LWasmUnalignedStoreI64* lir) {
  emitWasmStoreI64(lir);
}

void CodeGenerator::visitWasmSelectI64(LWasmSelectI64* lir) {
  MOZ_ASSERT(lir->mir()->type() == MIRType::Int64);

  Register cond = ToRegister(lir->condExpr());
  const LInt64Allocation falseExpr = lir->falseExpr();

  Register64 out = ToOutRegister64(lir);
  MOZ_ASSERT(ToRegister64(lir->trueExpr()) == out,
             "true expr is reused for input");

  if (falseExpr.value().isRegister()) {
    masm.as_movz(out.reg, ToRegister(falseExpr.value()), cond);
  } else {
    Label done;
    masm.ma_b(cond, cond, &done, Assembler::NonZero, ShortJump);
    masm.loadPtr(ToAddress(falseExpr.value()), out.reg);
    masm.bind(&done);
  }
}

void CodeGenerator::visitWasmReinterpretFromI64(LWasmReinterpretFromI64* lir) {
  MOZ_ASSERT(lir->mir()->type() == MIRType::Double);
  MOZ_ASSERT(lir->mir()->input()->type() == MIRType::Int64);
  masm.as_dmtc1(ToRegister(lir->input()), ToFloatRegister(lir->output()));
}

void CodeGenerator::visitWasmReinterpretToI64(LWasmReinterpretToI64* lir) {
  MOZ_ASSERT(lir->mir()->type() == MIRType::Int64);
  MOZ_ASSERT(lir->mir()->input()->type() == MIRType::Double);
  masm.as_dmfc1(ToRegister(lir->output()), ToFloatRegister(lir->input()));
}

void CodeGenerator::visitExtendInt32ToInt64(LExtendInt32ToInt64* lir) {
  const LAllocation* input = lir->getOperand(0);
  Register output = ToRegister(lir->output());

  if (lir->mir()->isUnsigned()) {
    masm.ma_dext(output, ToRegister(input), Imm32(0), Imm32(32));
  } else {
    masm.ma_sll(output, ToRegister(input), Imm32(0));
  }
}

void CodeGenerator::visitWrapInt64ToInt32(LWrapInt64ToInt32* lir) {
  const LAllocation* input = lir->getOperand(0);
  Register output = ToRegister(lir->output());

  if (lir->mir()->bottomHalf()) {
    if (input->isMemory()) {
      masm.load32(ToAddress(input), output);
    } else {
      masm.ma_sll(output, ToRegister(input), Imm32(0));
    }
  } else {
    MOZ_CRASH("Not implemented.");
  }
}

void CodeGenerator::visitSignExtendInt64(LSignExtendInt64* lir) {
  Register64 input = ToRegister64(lir->getInt64Operand(0));
  Register64 output = ToOutRegister64(lir);
  switch (lir->mode()) {
    case MSignExtendInt64::Byte:
      masm.move32To64SignExtend(input.reg, output);
      masm.move8SignExtend(output.reg, output.reg);
      break;
    case MSignExtendInt64::Half:
      masm.move32To64SignExtend(input.reg, output);
      masm.move16SignExtend(output.reg, output.reg);
      break;
    case MSignExtendInt64::Word:
      masm.move32To64SignExtend(input.reg, output);
      break;
  }
}

void CodeGenerator::visitClzI64(LClzI64* lir) {
  Register64 input = ToRegister64(lir->getInt64Operand(0));
  Register64 output = ToOutRegister64(lir);
  masm.clz64(input, output.reg);
}

void CodeGenerator::visitCtzI64(LCtzI64* lir) {
  Register64 input = ToRegister64(lir->getInt64Operand(0));
  Register64 output = ToOutRegister64(lir);
  masm.ctz64(input, output.reg);
}

void CodeGenerator::visitNotI64(LNotI64* lir) {
  Register64 input = ToRegister64(lir->getInt64Operand(0));
  Register output = ToRegister(lir->output());

  masm.cmp64Set(Assembler::Equal, input.reg, Imm32(0), output);
}

void CodeGenerator::visitWasmTruncateToInt64(LWasmTruncateToInt64* lir) {
  FloatRegister input = ToFloatRegister(lir->input());
  Register64 output = ToOutRegister64(lir);

  MWasmTruncateToInt64* mir = lir->mir();
  MIRType fromType = mir->input()->type();

  MOZ_ASSERT(fromType == MIRType::Double || fromType == MIRType::Float32);

  auto* ool = new (alloc()) OutOfLineWasmTruncateCheck(mir, input, output);
  addOutOfLineCode(ool, mir);

  Label* oolEntry = ool->entry();
  Label* oolRejoin = ool->rejoin();
  bool isSaturating = mir->isSaturating();

  if (fromType == MIRType::Double) {
    if (mir->isUnsigned()) {
      masm.wasmTruncateDoubleToUInt64(input, output, isSaturating, oolEntry,
                                      oolRejoin, InvalidFloatReg);
    } else {
      masm.wasmTruncateDoubleToInt64(input, output, isSaturating, oolEntry,
                                     oolRejoin, InvalidFloatReg);
    }
  } else {
    if (mir->isUnsigned()) {
      masm.wasmTruncateFloat32ToUInt64(input, output, isSaturating, oolEntry,
                                       oolRejoin, InvalidFloatReg);
    } else {
      masm.wasmTruncateFloat32ToInt64(input, output, isSaturating, oolEntry,
                                      oolRejoin, InvalidFloatReg);
    }
  }
}

void CodeGenerator::visitInt64ToFloatingPoint(LInt64ToFloatingPoint* lir) {
  Register64 input = ToRegister64(lir->getInt64Operand(0));
  FloatRegister output = ToFloatRegister(lir->output());

  MIRType outputType = lir->mir()->type();
  MOZ_ASSERT(outputType == MIRType::Double || outputType == MIRType::Float32);

  if (outputType == MIRType::Double) {
    if (lir->mir()->isUnsigned()) {
      masm.convertUInt64ToDouble(input, output, Register::Invalid());
    } else {
      masm.convertInt64ToDouble(input, output);
    }
  } else {
    if (lir->mir()->isUnsigned()) {
      masm.convertUInt64ToFloat32(input, output, Register::Invalid());
    } else {
      masm.convertInt64ToFloat32(input, output);
    }
  }
}

void CodeGenerator::visitTestI64AndBranch(LTestI64AndBranch* lir) {
  Register64 input = ToRegister64(lir->getInt64Operand(0));
  MBasicBlock* ifTrue = lir->ifTrue();
  MBasicBlock* ifFalse = lir->ifFalse();

  emitBranch(input.reg, Imm32(0), Assembler::NonZero, ifTrue, ifFalse);
}
