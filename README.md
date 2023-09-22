# PDG_demo
A toy implementation about Program Dependence Graph using LLVM by zhaosiying12138.

## Usage
### Step 1: Make a testfile
clang -Xclang -disable-O0-optnone -S -O0 -emit-llvm test.c -o test\_tmp.ll  
opt -S -simplifycfg test\_tmp.ll -o test\_tmp\_opt.ll  
opt -dot-cfg test\_tmp\_opt.ll > /dev/null  
dot -T png -o test\_cfg.png .zsy\_test.dot  
