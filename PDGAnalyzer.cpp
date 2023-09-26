#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/PostDominators.h"
#include "llvm/ADT/BreadthFirstIterator.h"
#include "llvm/IR/CFG.h"


using namespace llvm;

namespace {
class PDGAnalyzer : public PassInfoMixin<PDGAnalyzer> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // end anonymous namespace

PreservedAnalyses
PDGAnalyzer::run(Function &F, FunctionAnalysisManager &FAM) {
  llvm::outs() << "[ZSY] Hello world PDG! " << F.getName() << "\n";
  auto PA = PreservedAnalyses::all();

  PostDominatorTree &PDT = FAM.getResult<PostDominatorTreeAnalysis>(F);
  PDT.print(llvm::outs());
  llvm::outs() << "\n";

  for (const BasicBlock *BB : breadth_first(&F)) {
    const Instruction *TInst = BB->getTerminator();
    int branch_cnt = TInst->getNumSuccessors();
    if (branch_cnt != 2)
      continue;
    for (int i = 0; i < 2; i++) {
      const BasicBlock *BB_succ = TInst->getSuccessor(i);
      bool isPDom = PDT.dominates(BB_succ, BB);
      if (isPDom)
        continue;
      llvm::outs() << "(";
      llvm::outs() << BB->getName();
      llvm::outs() << ", ";
      llvm::outs() << BB_succ->getName();
      llvm::outs() << ")";
      llvm::outs() << "\t";

      llvm::outs() << BB->getName();
      llvm::outs() << "\t";

      if (i == 0) {
        llvm::outs() << "T";
      } else {
        llvm::outs() << "F";
      }
      llvm::outs() << "\t";

      llvm::outs() << "[";

      DomTreeNode *dtnode_end = PDT.getNode(BB)->getIDom();
      DomTreeNode *dtnode_iterator_BB_succ = PDT.getNode(BB_succ);
      while (dtnode_iterator_BB_succ != dtnode_end) {
        llvm::outs() << dtnode_iterator_BB_succ->getBlock()->getName();
        dtnode_iterator_BB_succ = dtnode_iterator_BB_succ->getIDom();
        if (dtnode_iterator_BB_succ != dtnode_end) {
          llvm::outs() << ", ";
        }
      }
      llvm::outs() << "]";

      llvm::outs() << "\n";
    }

  }

  return PA;
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "PDGAnalyzer", "v1.0",
    [](PassBuilder &PB) {
      using PipelineElement = typename PassBuilder::PipelineElement;
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PipelineElement>) {
	  if (Name == "pdg-analyzer") {
	    FPM.addPass(PDGAnalyzer());
	    return true;
	  }
	  return false;
        });
    }
  };
}
