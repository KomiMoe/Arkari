#ifndef OBFUSCATION_STRING_ENCRYPTION_H
#define OBFUSCATION_STRING_ENCRYPTION_H

namespace llvm {
class ModulePass;
class PassRegistry;
struct ObfuscationOptions;

ModulePass* createStringEncryptionPass();
ModulePass* createStringEncryptionPass(bool flag, ObfuscationOptions *Options);
void initializeStringEncryptionPass(PassRegistry &Registry);

}

#endif
