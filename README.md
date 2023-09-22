# PDG_demo
A toy implementation about Program Dependence Graph using LLVM by zhaosiying12138.

## Usage
clang -Xclang -disable-O0-optnone -S -O0 -emit-llvm test.c -o test_tmp.ll
opt -S -simplifycfg test_tmp.ll -o test_tmp_opt.ll
opt -dot-cfg test\_tmp\_opt.ll > /dev/null
dot -T png -o test_ssa_cfg.png .zsy_test.dot
