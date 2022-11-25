#ifndef OBFUSCATION_INDIRECTBR_H
#define OBFUSCATION_INDIRECTBR_H

// Namespace
namespace llvm {
class FunctionPass;
class PassRegistry;
struct ObfuscationOptions;

FunctionPass* createIndirectBranchPass();
FunctionPass* createIndirectBranchPass(bool flag, ObfuscationOptions *Options);
void initializeIndirectBranchPass(PassRegistry &Registry);

}

#endif
