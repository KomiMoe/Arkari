# goron
Yet another llvm based obfuscator.

当前支持特性：
 - 混淆过程间相关
 - 间接跳转,并加密跳转目标(-mllvm -irobf-indbr)
 - 间接函数调用,并加密目标函数地址(-mllvm -irobf-icall)
 - 间接全局变量引用,并加密变量地址(-mllvm -irobf-indgv)
 - 字符串(c string)加密功能(-mllvm -irobf-cse)
 - 过程相关控制流平坦混淆(-mllvm -irobf-cff)
 - 全部 ( -mllvm -irobf-indbr -mllvm -irobf-icall -mllvm -irobf-indgv -mllvm -irobf-cse -mllvm -irobf-cff )

## 编译

```
mkdir build_ninja
cd build_ninja
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="clang;lld" -G "Ninja" ../llvm
ninja
```

## 使用
跟ollvm类似,可通过编译选项开启相应混淆.
如启用间接跳转混淆
```
$ path_to_the/build/bin/clang -mllvm -irobf -mllvm --irobf-indbr test.c
```
对于使用autotools的工程
```
$ CC=path_to_the/build/bin/clang or CXX=path_to_the/build/bin/clang
$ CFLAGS+="-mllvm -irobf -mllvm --irobf-indbr" or CXXFLAGS+="-mllvm -irobf -mllvm --irobf-indbr" (or any other obfuscation-related flags)
$ ./configure
$ make
```

## 参考资源
+ [Hikari](https://github.com/HikariObfuscator/Hikari)
+ [ollvm](https://github.com/obfuscator-llvm/obfuscator)
