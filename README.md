# PDG\_demo  
A toy implementation about Program Dependence Graph using LLVM by zhaosiying12138.  

## File Organization  
demo/test.c : original C test file  
demo/test\_opt.ll & test\_opt\_cfg.png: the LLVM IR & CFG generated by Clang and Graphviz  
demo/test\_opt\_with\_header.ll & demo/test\_opt\_with\_header\_cfg.png: add unique ENTRY and STOP Basic Block to test\_opt.ll [manually]  

## Usage  
### Step 1: Make a testfile  
clang -Xclang -disable-O0-optnone -S -O0 -emit-llvm test.c -o test\_tmp.ll  
opt -S -simplifycfg test\_tmp.ll -o test\_opt.ll  
opt -dot-cfg test\_opt.ll > /dev/null  
dot -T png -o test\_opt\_cfg.png .zsy\_test.dot  

### Step 2: Build Pass  
mkdir build  
cd build  
cmake ..  
make -j 65535  

### Step 3: Call opt to enable our PDG-Pass to analysis LLVM IR and construct the Control Dependence Graph  
opt --enable-new-pm --load-pass-plugin /your\_path\_to\_PDG\_demo/build/HaltAnalyzer.so --passes="halt-analyzer" --disable-output /your\_path\_to\_PDG\_demo/demo/test\_opt\_with\_header.ll  

## Acknownledgement  
Thanks for the help of https://github.com/PacktPublishing/LLVM-Techniques-Tips-and-Best-Practices-Clang-and-Middle-End-Libraries/tree/main/Chapter09 to let me get started in writing the LLVM Pass from ZERO!  
This is my first LLVM Pass written in my life!!!  
