# Arkari
Yet another llvm based obfuscator based on [goron](https://github.com/amimo/goron).

当前支持特性：
 - 混淆过程间相关
 - 间接跳转,并加密跳转目标(-mllvm -irobf-indbr)
 - 间接函数调用,并加密目标函数地址(-mllvm -irobf-icall)
 - 间接全局变量引用,并加密变量地址(-mllvm -irobf-indgv)
 - 字符串(c string)加密功能(-mllvm -irobf-cse)
 - 过程相关控制流平坦混淆(-mllvm -irobf-cff)
 - 全部 (-mllvm -irobf-indbr -mllvm -irobf-icall -mllvm -irobf-indgv -mllvm -irobf-cse -mllvm -irobf-cff)

对比于goron的改进：
 - 由于作者明确表示暂时(至少几万年吧)不会跟进llvm版本和不会继续更新. 所以有了这个版本(https://github.com/amimo/goron/issues/29)
 - 更新了llvm版本
 - 编译时输出文件名, 防止憋死强迫症
 - 修复了亿点点已知的bug
 ```
 - 修复了混淆后SEH爆炸的问题
 - 修复了dll导入的全局变量会被混淆导致丢失__impl前缀的问题
 - 修复了某些情况下配合llvm2019(2022)插件会导致参数重复添加无法编译的问题
 - 修复了x86间接调用炸堆栈的问题
 - ...
 ```
## 编译

 - Windows(use Ninja, Ninja YYDS):
```
install ninja in your PATH
run x64(86) Native Tools Command Prompt for VS 2022(xx)
run:

mkdir build_ninja
cd build_ninja
cmake -DCMAKE_CXX_FLAGS="/utf-8" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld;lldb" -G "Ninja" ../llvm
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
+ [Goron](https://github.com/amimo/goron)
+ [Hikari](https://github.com/HikariObfuscator/Hikari)
+ [ollvm](https://github.com/obfuscator-llvm/obfuscator)
