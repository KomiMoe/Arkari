//===- Flattening.cpp - Flattening Obfuscation pass------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the flattening pass
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Obfuscation/Flattening.h"
#include "llvm/Transforms/Obfuscation/LegacyLowerSwitch.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/CryptoUtils.h"
#include "llvm/ADT/Statistic.h"

#define DEBUG_TYPE "flattening"

using namespace std;
using namespace llvm;

// Stats
STATISTIC(Flattened, "Functions flattened");

namespace {
struct Flattening : public FunctionPass {
  unsigned pointerSize;
  static char ID;  // Pass identification, replacement for typeid
  bool flag;
  
  ObfuscationOptions *Options;
  CryptoUtils RandomEngine;

  Flattening(unsigned pointerSize) : FunctionPass(ID) {
    this->pointerSize = pointerSize;
    this->flag = false;
    this->Options = nullptr;
  }

  Flattening(unsigned pointerSize, bool flag, ObfuscationOptions *Options) : FunctionPass(ID) {
    this->pointerSize = pointerSize;
    this->flag = flag;
    this->Options = Options;
  }

  bool runOnFunction(Function &F);
  bool flatten(Function *f);
};
}

bool Flattening::runOnFunction(Function &F) {
  Function *tmp = &F;
  bool result = false;
  // Do we obfuscate
  if (toObfuscate(flag, tmp, "fla")) {
    if (flatten(tmp)) {
      ++Flattened;
      result = true;
    }
  }

  return result;
}

bool Flattening::flatten(Function *f) {
  vector<BasicBlock *> origBB;
  BasicBlock *loopEntry;
  BasicBlock *loopEnd;
  LoadInst *load;
  SwitchInst *switchI;
  AllocaInst *switchVar;

  // SCRAMBLER
  char scrambling_key[16];
  llvm::cryptoutils->get_bytes(scrambling_key, 16);
  // END OF SCRAMBLER

  // Lower switch
  FunctionPass *lower = createLegacyLowerSwitchPass();
  lower->runOnFunction(*f);

  // Save all original BB
  for (Function::iterator i = f->begin(); i != f->end(); ++i) {
    BasicBlock *tmp = &*i;
    origBB.push_back(tmp);

    BasicBlock *bb = &*i;
    if (isa<InvokeInst>(bb->getTerminator())) {
      return false;
    }
  }

  // Nothing to flatten
  if (origBB.size() <= 1) {
    return false;
  }

  LLVMContext &Ctx = f->getContext();
  IntegerType* intType = Type::getInt32Ty(Ctx);
  if (pointerSize == 8) {
    intType = Type::getInt64Ty(Ctx);
  }

  Value *MySecret = ConstantInt::get(intType, 0, true);

  // Remove first BB
  origBB.erase(origBB.begin());

  // Get a pointer on the first BB
  Function::iterator tmp = f->begin();  //++tmp;
  BasicBlock *insert = &*tmp;

  // If main begin with an if
  BranchInst *br = NULL;
  if (isa<BranchInst>(insert->getTerminator())) {
    br = cast<BranchInst>(insert->getTerminator());
  }

  if ((br != NULL && br->isConditional()) ||
      insert->getTerminator()->getNumSuccessors() > 1) {
    BasicBlock::iterator i = insert->end();
        --i;

    if (insert->size() > 1) {
      --i;
    }

    BasicBlock *tmpBB = insert->splitBasicBlock(i, "first");
    origBB.insert(origBB.begin(), tmpBB);
  }

  // Remove jump
  insert->getTerminator()->eraseFromParent();

  // Create switch variable and set as it
  switchVar =
      new AllocaInst(intType, 0, "switchVar", insert);
  if (pointerSize == 8) {
    new StoreInst(
      ConstantInt::get(intType,
        llvm::cryptoutils->scramble64(0, scrambling_key)),
      switchVar, insert);
  } else {
    new StoreInst(
      ConstantInt::get(intType,
        llvm::cryptoutils->scramble32(0, scrambling_key)),
      switchVar, insert);
  }

  // Create main loop
  loopEntry = BasicBlock::Create(f->getContext(), "loopEntry", f, insert);
  loopEnd = BasicBlock::Create(f->getContext(), "loopEnd", f, insert);

  load = new LoadInst(intType, switchVar, "switchVar", loopEntry);

  // Move first BB on top
  insert->moveBefore(loopEntry);
  BranchInst::Create(loopEntry, insert);

  // loopEnd jump to loopEntry
  BranchInst::Create(loopEntry, loopEnd);

  BasicBlock *swDefault =
      BasicBlock::Create(f->getContext(), "switchDefault", f, loopEnd);
  BranchInst::Create(loopEnd, swDefault);

  // Create switch instruction itself and set condition
  switchI = SwitchInst::Create(&*f->begin(), swDefault, 0, loopEntry);
  switchI->setCondition(load);

  // Remove branch jump from 1st BB and make a jump to the while
  f->begin()->getTerminator()->eraseFromParent();

  BranchInst::Create(loopEntry, &*f->begin());

  // Put all BB in the switch
  for (vector<BasicBlock *>::iterator b = origBB.begin(); b != origBB.end();
       ++b) {
    BasicBlock *i = *b;
    ConstantInt *numCase = NULL;

    // Move the BB inside the switch (only visual, no code logic)
    i->moveBefore(loopEnd);

    // Add case to switch
    if (pointerSize == 8) {
      numCase = cast<ConstantInt>(ConstantInt::get(
          switchI->getCondition()->getType(),
          llvm::cryptoutils->scramble64(switchI->getNumCases(), scrambling_key)));
    } else {
      numCase = cast<ConstantInt>(ConstantInt::get(
        switchI->getCondition()->getType(),
        llvm::cryptoutils->scramble32(switchI->getNumCases(), scrambling_key)));
    }
    switchI->addCase(numCase, i);
  }

  ConstantInt *Zero = ConstantInt::get(intType, 0);
  // Recalculate switchVar
  for (vector<BasicBlock *>::iterator b = origBB.begin(); b != origBB.end();
       ++b) {
    BasicBlock *i = *b;
    ConstantInt *numCase = NULL;

    // Ret BB
    if (i->getTerminator()->getNumSuccessors() == 0) {
      continue;
    }

    // If it's a non-conditional jump
    if (i->getTerminator()->getNumSuccessors() == 1) {
      // Get successor and delete terminator
      BasicBlock *succ = i->getTerminator()->getSuccessor(0);
      i->getTerminator()->eraseFromParent();

      // Get next case
      numCase = switchI->findCaseDest(succ);

      // If next case == default case (switchDefault)
      if (numCase == NULL) {
        if (pointerSize == 8) {
          numCase = cast<ConstantInt>(
              ConstantInt::get(switchI->getCondition()->getType(),
                               llvm::cryptoutils->scramble64(
                                   switchI->getNumCases() - 1, scrambling_key)));
        } else {
          numCase = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
              llvm::cryptoutils->scramble32(
                switchI->getNumCases() - 1, scrambling_key)));
        }
      }

      // numCase = MySecret - (MySecret - numCase)
      // X = MySecret - numCase
      Constant *X = ConstantExpr::getSub(Zero, numCase);
      Value *newNumCase = BinaryOperator::Create(Instruction::Sub, MySecret, X, "", i);

      // Update switchVar and jump to the end of loop
      new StoreInst(newNumCase, load->getPointerOperand(), i);
      BranchInst::Create(loopEnd, i);
      continue;
    }

    // If it's a conditional jump
    if (i->getTerminator()->getNumSuccessors() == 2) {
      // Get next cases
      ConstantInt *numCaseTrue =
          switchI->findCaseDest(i->getTerminator()->getSuccessor(0));
      ConstantInt *numCaseFalse =
          switchI->findCaseDest(i->getTerminator()->getSuccessor(1));

      // Check if next case == default case (switchDefault)
      if (numCaseTrue == NULL) {
        if (pointerSize == 8) {
          numCaseTrue = cast<ConstantInt>(
              ConstantInt::get(switchI->getCondition()->getType(),
                               llvm::cryptoutils->scramble64(
                                   switchI->getNumCases() - 1, scrambling_key)));
        } else {
          numCaseTrue = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
              llvm::cryptoutils->scramble32(
                switchI->getNumCases() - 1, scrambling_key)));
        }
      }

      if (numCaseFalse == NULL) {
        if (pointerSize == 8) {
          numCaseFalse = cast<ConstantInt>(
              ConstantInt::get(switchI->getCondition()->getType(),
                               llvm::cryptoutils->scramble64(
                                   switchI->getNumCases() - 1, scrambling_key)));
        } else {
          numCaseFalse = cast<ConstantInt>(
            ConstantInt::get(switchI->getCondition()->getType(),
              llvm::cryptoutils->scramble32(
                switchI->getNumCases() - 1, scrambling_key)));
        }
      }

      Constant *X, *Y;
      X = ConstantExpr::getSub(Zero, numCaseTrue);
      Y = ConstantExpr::getSub(Zero, numCaseFalse);
      Value *newNumCaseTrue = BinaryOperator::Create(Instruction::Sub, MySecret, X, "", i->getTerminator());
      Value *newNumCaseFalse = BinaryOperator::Create(Instruction::Sub, MySecret, Y, "", i->getTerminator());

      // Create a SelectInst
      BranchInst *br = cast<BranchInst>(i->getTerminator());
      SelectInst *sel =
          SelectInst::Create(br->getCondition(), newNumCaseTrue, newNumCaseFalse, "",
                             i->getTerminator());

      // Erase terminator
      i->getTerminator()->eraseFromParent();

      // Update switchVar and jump to the end of loop
      new StoreInst(sel, load->getPointerOperand(), i);
      BranchInst::Create(loopEnd, i);
      continue;
    }
  }

  fixStack(f);

  lower->runOnFunction(*f);
  delete(lower);

  return true;
}

char Flattening::ID = 0;
static RegisterPass<Flattening> X("flattening", "Call graph flattening");
FunctionPass *llvm::createFlatteningPass(unsigned pointerSize) { return new Flattening(pointerSize); }
FunctionPass *llvm::createFlatteningPass(unsigned pointerSize, bool flag, ObfuscationOptions *Options) {
  return new Flattening(pointerSize, flag, Options);
}
