#ifndef OBFUSCATION_INDIRECTBR_H
#define OBFUSCATION_INDIRECTBR_H

// Namespace
namespace llvm {
class FunctionPass;
class PassRegistry;
struct ObfuscationOptions;

FunctionPass* createIndirectBranchPass(unsigned pointerSize);
FunctionPass* createIndirectBranchPass(unsigned pointerSize, bool flag, ObfuscationOptions *Options);
void initializeIndirectBranchPass(PassRegistry &Registry);

}

#endif
