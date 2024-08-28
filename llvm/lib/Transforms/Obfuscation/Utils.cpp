#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DiagnosticInfo.h"

// Shamefully borrowed from ../Scalar/RegToMem.cpp :(
bool valueEscapes(Instruction *Inst) {
  BasicBlock *BB = Inst->getParent();
  for (Value::use_iterator UI = Inst->use_begin(), E = Inst->use_end(); UI != E;
       ++UI) {
    Instruction *I = cast<Instruction>(*UI);
    if (I->getParent() != BB || isa<PHINode>(I)) {
      return true;
    }
  }
  return false;
}

void fixStack(Function *f) {
  // Try to remove phi node and demote reg to stack
  std::vector<PHINode *> tmpPhi;
  std::vector<Instruction *> tmpReg;
  BasicBlock *bbEntry = &*f->begin();

  do {
    tmpPhi.clear();
    tmpReg.clear();

    for (Function::iterator i = f->begin(); i != f->end(); ++i) {

      for (BasicBlock::iterator j = i->begin(); j != i->end(); ++j) {

        if (isa<PHINode>(j)) {
          PHINode *phi = cast<PHINode>(j);
          tmpPhi.push_back(phi);
          continue;
        }
        if (!(isa<AllocaInst>(j) && j->getParent() == bbEntry) &&
            (valueEscapes(&*j) || j->isUsedOutsideOfBlock(&*i))) {
          tmpReg.push_back(&*j);
          continue;
        }
      }
    }
    for (unsigned int i = 0; i != tmpReg.size(); ++i) {
      DemoteRegToStack(*tmpReg.at(i));
    }

    for (unsigned int i = 0; i != tmpPhi.size(); ++i) {
      DemotePHIToStack(tmpPhi.at(i));
    }

  } while (tmpReg.size() != 0 || tmpPhi.size() != 0);
}

SmallVector<std::string> readAnnotate(Function *f) {
  SmallVector<std::string> annotations;

  auto* Annotations = f->getParent()->getGlobalVariable("llvm.global.annotations");
  auto* C = dyn_cast_or_null<Constant>(Annotations);
  if (!C || C->getNumOperands() != 1)
    return annotations;

  C = cast<Constant>(C->getOperand(0));

  // Iterate over all entries in C and attach !annotation metadata to suitable
  // entries.
  for (auto& Op : C->operands()) {
    // Look at the operands to check if we can use the entry to generate
    // !annotation metadata.
    auto* OpC = dyn_cast<ConstantStruct>(&Op);
    if (!OpC || OpC->getNumOperands() < 2)
      continue;
    auto* Fn = dyn_cast<Function>(OpC->getOperand(0)->stripPointerCasts());
    if (Fn != f)
      continue;
    auto* StrC = dyn_cast<GlobalValue>(OpC->getOperand(1)->stripPointerCasts());
    if (!StrC)
      continue;
    auto* StrData = dyn_cast<ConstantDataSequential>(StrC->getOperand(0));
    if (!StrData)
      continue;
    annotations.emplace_back(StrData->getAsString());
  }
  return annotations;
}

bool toObfuscate(bool flag, Function *f, const std::string &attribute) {
  std::string attr = "+" + attribute;
  std::string attrNo = "-" + attribute;

  // Check if declaration
  if (f->isDeclaration()) {
    return false;
  }

  // Check external linkage
  if(f->hasAvailableExternallyLinkage() != 0) {
    return false;
  }

  // We have to check the nofla flag first
  // Because .find("fla") is true for a string like "fla" or
  // "nofla"
  bool annotationEnableFound = false;
  bool annotationDisableFound = false;

  auto annotations = readAnnotate(f);
  if (!annotations.empty()) {
    for (const auto& annotation : annotations) {
      if (annotation.find(attrNo) != std::string::npos) {
        annotationDisableFound = true;
      }
      if (annotation.find(attr) != std::string::npos) {
        annotationEnableFound = true;
      }
    }
  }

  if (annotationDisableFound && annotationEnableFound) {
     f->getContext().diagnose(DiagnosticInfoUnsupported{ *f, f->getName() + " having both enable annotation and disable annotation, What are you the fucking want to do?" });
  }

  return annotationEnableFound ? true : annotationDisableFound ? false : flag;
//   // If fla flag is set
//   if (flag == true) {
//     /* Check if the number of applications is correct
//     if (!((Percentage > 0) && (Percentage <= 100))) {
//       LLVMContext &ctx = llvm::getGlobalContext();
//       ctx.emitError(Twine("Flattening application function\
//               percentage -perFLA=x must be 0 < x <= 100"));
//     }
//     // Check name
//     else if (func.size() != 0 && func.find(f->getName()) != std::string::npos) {
//       return true;
//     }
//
//     if ((((int)llvm::cryptoutils->get_range(100))) < Percentage) {
//       return true;
//     }
//     */
//     return true;
//   }
//
//   return false;
}

void LowerConstantExpr(Function &F) {
  SmallPtrSet<Instruction *, 8> WorkList;

  for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It) {
    Instruction *I = &*It;

    if (isa<LandingPadInst>(I) || isa<CatchPadInst>(I) || isa<CatchSwitchInst>(I) || isa<CatchReturnInst>(I))
      continue;
    if (auto *II = dyn_cast<IntrinsicInst>(I)) {
      if (II->getIntrinsicID() == Intrinsic::eh_typeid_for) {
        continue;
      }
    }

    for (unsigned int i = 0; i < I->getNumOperands(); ++i) {
      if (isa<ConstantExpr>(I->getOperand(i)))
        WorkList.insert(I);
    }
  }

  while (!WorkList.empty()) {
    auto It = WorkList.begin();
    Instruction *I = *It;
    WorkList.erase(*It);

    if (PHINode *PHI = dyn_cast<PHINode>(I)) {
      for (unsigned int i = 0; i < PHI->getNumIncomingValues(); ++i) {
        Instruction *TI = PHI->getIncomingBlock(i)->getTerminator();
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(PHI->getIncomingValue(i))) {
          Instruction *NewInst = CE->getAsInstruction();
          NewInst->insertBefore(TI);
          PHI->setIncomingValue(i, NewInst);
          WorkList.insert(NewInst);
        }
      }
    } else {
      for (unsigned int i = 0; i < I->getNumOperands(); ++i) {
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(I->getOperand(i))) {
          Instruction *NewInst = CE->getAsInstruction();
          NewInst->insertBefore(I);
          I->replaceUsesOfWith(CE, NewInst);
          WorkList.insert(NewInst);
        }
      }
    }
  }
}
