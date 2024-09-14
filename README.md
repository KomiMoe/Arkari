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
可通过编译选项开启相应混淆，如启用间接跳转混淆：

```
$ path_to_the/build/bin/clang -mllvm -irobf -mllvm --irobf-indbr test.c
```
对于使用autotools的工程：
```
$ CC=path_to_the/build/bin/clang or CXX=path_to_the/build/bin/clang
$ CFLAGS+="-mllvm -irobf -mllvm --irobf-indbr" or CXXFLAGS+="-mllvm -irobf -mllvm --irobf-indbr" (or any other obfuscation-related flags)
$ ./configure
$ make
```

可以通过**annotate**对特定函数**开启/关闭**指定混淆选项：

```cpp
//fla表示编译选项中的cff
[[clang::annotate("-fla -icall")]] int foo(auto a, auto b){
    return a + b;
}

[[clang::annotate("+indbr +cse")]] int main(int argc, char** argv) {
    foo(1, 2);
    std::printf("hello clang\n");

    return 0;
}
// 当然如果你不嫌麻烦也可以用 __attribute((__annotate__(("+indbr +cse"))))
```

如果你不希望对整个程序都启用Pass，那么你可以在编译选项中只添加 **-mllvm -irobf** ，然后使用 **annotate** 控制需要混淆的函数，需要注意的是仅开启 **-irobf** 不使用 **annotate** 不会运行任何混淆Pass，当然，不添加任何混淆参数的情况下，仅使用 **annotate** 也不会启用任何Pass

当然以下情况会报错：

```cpp
[[clang::annotate("-fla +fla")]] int fool(auto a, auto b){
    return a + b;
}
```

你**不能**同时开启和关闭某个混淆参数！

## 参考资源

+ [Goron](https://github.com/amimo/goron)
+ [Hikari](https://github.com/HikariObfuscator/Hikari)
+ [ollvm](https://github.com/obfuscator-llvm/obfuscator)
