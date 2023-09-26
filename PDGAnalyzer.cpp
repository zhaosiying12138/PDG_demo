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
#include <map>

using namespace llvm;

namespace {
class PDGAnalyzer : public PassInfoMixin<PDGAnalyzer> {

  struct control_dep_info {
    BasicBlock *BB;
    bool condition;
  };
  std::multimap<BasicBlock *, control_dep_info> cdg_map;
  void traverse_PDT_in_post_order(DomTreeNode *node);

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // end anonymous namespace

void PDGAnalyzer::traverse_PDT_in_post_order(DomTreeNode *node)
{
  for (DomTreeNode *child : node->children()) {
    traverse_PDT_in_post_order(child);
  }
  llvm::outs() << node->getBlock()->getName() << "\n";
}

PreservedAnalyses
PDGAnalyzer::run(Function &F, FunctionAnalysisManager &FAM) {
  llvm::outs() << "[ZSY] Hello world PDG! " << F.getName() << "\n";
  auto PA = PreservedAnalyses::all();

  PostDominatorTree &PDT = FAM.getResult<PostDominatorTreeAnalysis>(F);
  PDT.print(llvm::outs());
  llvm::outs() << "\n";

  for (BasicBlock *BB : breadth_first(&F)) {
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
        control_dep_info tmp_cd_info;
        tmp_cd_info.BB = BB;
        tmp_cd_info.condition = i == 0;
        cdg_map.emplace(dtnode_iterator_BB_succ->getBlock(), tmp_cd_info);

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

  llvm::outs() << "\n";
  std::error_code ec;
  llvm::raw_fd_ostream dot_outs("../demo/zsy_test_cdg.dot", ec);
  dot_outs << "digraph G {\n";
  dot_outs << "\tedge[style=dashed];\n";
  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                               IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock *BB = *I;
    auto it = cdg_map.equal_range(BB);
    if (it.first != cdg_map.end() && BB->getName() != "START" && BB->getName() != "ENTRY") {
      llvm::outs() << BB->getName() << " [CONTROL DEPEND ON]: ";
      for (auto itt = it.first; itt != it.second; ++itt) {
        control_dep_info tmp_cd_info = itt->second;
        llvm::outs() << tmp_cd_info.BB->getName() << "-";
        if (tmp_cd_info.condition) {
          llvm::outs() << "T";
        } else {
          llvm::outs() << "F";
        }
        llvm::outs() << "\t";
      }
      llvm::outs() << "\n";
    }
  }

  dot_outs << "}\n";

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
