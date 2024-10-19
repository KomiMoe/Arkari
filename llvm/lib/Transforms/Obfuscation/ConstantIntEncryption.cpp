#include "llvm/Transforms/Obfuscation/ObfuscationOptions.h"
#include "llvm/Transforms/Obfuscation/ConstantIntEncryption.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/Transforms/Utils/GlobalStatus.h"
#include "llvm/Transforms/IPO/Attributor.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CryptoUtils.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <map>
#include <set>
#include <iostream>
#include <algorithm>

#define DEBUG_TYPE "constant-int-encryption"

using namespace llvm;
namespace {

  struct ConstantIntEncryption : public FunctionPass {
    static char ID;
    ObfuscationOptions *ArgsOptions;
    CryptoUtils RandomEngine;

    ConstantIntEncryption(ObfuscationOptions *argsOptions) : FunctionPass(ID) {
      this->ArgsOptions = argsOptions;
    }

    StringRef getPassName() const override { return {"ConstantIntEncryption"}; }

    bool runOnFunction(Function &F) override {
      const auto opt = ArgsOptions->toObfuscate(ArgsOptions->cieOpt(), &F);
      if (!opt.isEnabled()) {
        return false;
      }

      bool Changed = false;
      LLVMContext &Ctx = F.getContext();
      IRBuilder<NoFolder> IRB(Ctx);

      for (auto& BB : F) {
        for (auto& I : BB) {
          if (I.isEHPad() || isa<AllocaInst>(&I) || isa<IntrinsicInst>(&I) ||
            isa<SwitchInst>(&I) || I.isAtomic() ||
            isa<PHINode>(&I)) {
            continue;
          }
          auto CI = dyn_cast<CallInst>(&I);
          auto GEP = dyn_cast<GetElementPtrInst>(&I);
          auto IsPhi = isa<PHINode>(&I);
          auto InsertPt = IsPhi ? F.getEntryBlock().getFirstInsertionPt() : I.getIterator();

          for (unsigned i = 0; i < I.getNumOperands(); ++i) {
            if (CI && CI->isBundleOperand(i)) {
              continue;
            }
            if (GEP && (i < 2 || GEP->getSourceElementType()->isStructTy())) {
              continue;
            }
            auto Opr = I.getOperand(i);
            if (auto CIT = dyn_cast<ConstantInt>(Opr)) {
              IRB.SetInsertPoint(InsertPt);
              auto Key = ConstantInt::get(Opr->getType(), RandomEngine.get_uint64_t());
              auto Enc = ConstantExpr::getSub(CIT, Key);
              auto GV = new GlobalVariable(*F.getParent(), Enc->getType(), false, GlobalValue::LinkageTypes::PrivateLinkage, Enc);
              appendToCompilerUsed(*F.getParent(), {GV});
              //outs() << I << " ->\n";
              auto Load = IRB.CreateLoad(Enc->getType(), GV);
              auto NewOpr = IRB.CreateAdd(Key, Load);
              I.setOperand(i, NewOpr);
              //outs() << I << "\n\n";
              Changed = true;
            }
          }

        }
      }
      return Changed;
    }
  };
} // namespace llvm

char ConstantIntEncryption::ID = 0;
FunctionPass *llvm::createConstantIntEncryptionPass(ObfuscationOptions *argsOptions) {
  return new ConstantIntEncryption(argsOptions);
}

INITIALIZE_PASS(ConstantIntEncryption, "cie", "Enable IR Constant Integer Encryption", false, false)
