//===- FlatteningIncludes.h - Flattening Obfuscation pass------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains includes and defines for the flattening pass
//
//===----------------------------------------------------------------------===//

#ifndef _FLATTENING_INCLUDES_
#define _FLATTENING_INCLUDES_

namespace llvm {
class FunctionPass;
struct ObfuscationOptions;
FunctionPass *createFlatteningPass(unsigned pointerSize);
FunctionPass *createFlatteningPass(unsigned pointerSize, bool flag, ObfuscationOptions *Options);
}

#endif

