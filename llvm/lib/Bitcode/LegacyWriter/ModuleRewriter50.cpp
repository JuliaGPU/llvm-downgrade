//===- ValueEnumerator50.cpp - Rewrite IR for LLVM 5.0 ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ModuleRewriter50 class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Bitcode/BitcodeWriter.h"
#include "PointerRewriter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
using namespace llvm;

// LLVM 19 made va_start/va_end/va_copy take an explicit pointer type, mangling
// their names with a pointer suffix (e.g. llvm.va_start.p0). LLVM 5/7 only know
// these intrinsics under their unmangled names, so rename the declarations (and
// thereby all calls) back to the legacy form.
static bool renameVarargIntrinsics(Module &M) {
  bool Changed = false;
  for (Function &F : M) {
    if (!F.isIntrinsic())
      continue;
    StringRef Name;
    switch (F.getIntrinsicID()) {
    case Intrinsic::vastart: Name = "llvm.va_start"; break;
    case Intrinsic::vaend:   Name = "llvm.va_end";   break;
    case Intrinsic::vacopy:  Name = "llvm.va_copy";  break;
    default: continue;
    }
    if (F.getName() != Name) {
      F.setName(Name);
      Changed = true;
    }
  }
  return Changed;
}

static bool removeFreeze(Module &M) {
    // Find freeze instructions
    SmallVector<FreezeInst *, 8> Worklist;
    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (auto *FI = dyn_cast<FreezeInst>(&I)) {
                    Worklist.push_back(FI);
                }
            }
        }
    }
    if (Worklist.empty())
        return false;

    // Replace freeze instructions by their operand
    for (FreezeInst *FI : Worklist) {
        FI->replaceAllUsesWith(FI->getOperand(0));
        FI->eraseFromParent();
    }
    return true;
}

static bool replaceFNeg(Module &M) {
  // Find fneg instructions
  SmallVector<UnaryOperator *, 8> Worklist;
  for (Function &F : M)
    for (BasicBlock &BB : F)
      for (Instruction &I : BB)
        if (auto *Op = dyn_cast<UnaryOperator>(&I))
          if (Op->getOpcode() == Instruction::FNeg)
            Worklist.push_back(Op);
  if (Worklist.empty())
    return false;

  // Replace fneg instructions by fsub instructions
  IRBuilder<> Builder(M.getContext());
  for (UnaryOperator *Op : Worklist) {
    Builder.SetInsertPoint(Op);
    Value *In = Op->getOperand(0);
    Value *Zero = ConstantFP::get(In->getType(), -0.0);
    Op->replaceAllUsesWith(Builder.CreateFSub(Zero, In));
    Op->eraseFromParent();
  }
  return true;
}

bool BitcodeWriter50::prepareModule(Module &M) {
  bool Changed = removeFreeze(M);
  Changed |= replaceFNeg(M);
  Changed |= renameVarargIntrinsics(M);

  PointerRewriter PR(M);
  Changed |= PR.run();

  return Changed;
}
