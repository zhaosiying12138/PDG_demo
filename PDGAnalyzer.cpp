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
#include <vector>
#include <map>
#include <list>

using namespace llvm;

namespace {
class PDGAnalyzer : public PassInfoMixin<PDGAnalyzer> {
  struct control_dep_info {
    BasicBlock *BB;
    bool condition;
  };
  std::multimap<BasicBlock *, control_dep_info> cdg_map;

  struct RegionNode;
  struct CDGNode {
    int type;
    // type = 0 means BasicBlock
    BasicBlock *BB;
    bool condition;
    //type = 1 means RegionNode
    RegionNode *RN;
  };
  struct RegionNode {
    int id;
    std::list<CDGNode> data;
  };
  int total_region_node_id = 1;

  std::vector<RegionNode *> region_record;
  std::map<BasicBlock *, RegionNode *> map_bb_region;

  void zsy_dump_region_node_data(std::list<CDGNode> data);
  RegionNode *cdg_is_equal_region_node(std::list<CDGNode> data)
  {
    return NULL;
  }

  RegionNode *cdg_alloc_region_node(std::list<CDGNode> data)
  {
    RegionNode *new_region_node = new RegionNode();
    new_region_node->id = total_region_node_id++;
    new_region_node->data = data;
    region_record.push_back(new_region_node);
    return new_region_node;
  }

  void zsy_dump_region_node(RegionNode *region_node)
  {
    llvm::outs() << "R" << region_node->id << ": ";
    zsy_dump_region_node_data(region_node->data);
  }

  void zsy_dump_map_bb_region()
  {
    llvm::outs() << "Dump BasicBlock-RegionNode Mapping Relationship:\n";
    for (auto &it : map_bb_region) {
      llvm::outs() << it.first->getName() << ": R" << it.second->id << "\n";
    }
  }

  void zsy_dump_region_record()
  {
    llvm::outs() << "Dump All RegionNode Information:\n";
    for (RegionNode *rn : region_record) {
      zsy_dump_region_node(rn);
    }
  }

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
} // end anonymous namespace

void PDGAnalyzer::zsy_dump_region_node_data(std::list<CDGNode> data)
{
  for (CDGNode cdg_node : data) {
    if (cdg_node.type == 0) {
      llvm::outs() << cdg_node.BB->getName() << "-";
      if (cdg_node.condition)
        llvm::outs() << "T";
      else
        llvm::outs() << "F";
      llvm::outs() << ", ";
    }
  }
  llvm::outs() << "\n";
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


  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                               IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock *BB = *I;
    auto it = cdg_map.equal_range(BB);
    if (it.first != cdg_map.end() && BB->getName() != "START" &&
     BB->getName() != "STOP" && BB->getName() != "ENTRY") {
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

  llvm::outs() << "\n";
  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                               IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock *BB = *I;
    auto it = cdg_map.equal_range(BB);
    std::list<CDGNode> tmp_cdg_region_data;
    if (it.first != cdg_map.end() && BB->getName() != "START" &&
     BB->getName() != "STOP" && BB->getName() != "ENTRY") {
      // llvm::outs() << BB->getName() << " [CONTROL DEPEND ON]: ";
      for (auto itt = it.first; itt != it.second; ++itt) {
        control_dep_info tmp_cd_info = itt->second;
        CDGNode tmp_cdg_node;
        tmp_cdg_node.type = 0;
        tmp_cdg_node.BB = tmp_cd_info.BB;
        tmp_cdg_node.condition = tmp_cd_info.condition;
        tmp_cdg_region_data.push_back(tmp_cdg_node);
      }
      RegionNode *cdg_reuse_region = cdg_is_equal_region_node(tmp_cdg_region_data);
      if (cdg_reuse_region == NULL) {
        cdg_reuse_region = cdg_alloc_region_node(tmp_cdg_region_data);
        map_bb_region.emplace(BB, cdg_reuse_region);
      }
      
    }
  }
  zsy_dump_map_bb_region();
  llvm::outs() << "\n";
  zsy_dump_region_record();




  std::error_code ec;
  llvm::raw_fd_ostream cdg_outs("../demo/zsy_test_cdg.dot", ec);
  cdg_outs << "digraph G {\n";
  cdg_outs << "\tedge[style=dashed];\n";

  cdg_outs << "\n";
  for (po_iterator<BasicBlock *> I = po_begin(&F.getEntryBlock()),
                               IE = po_end(&F.getEntryBlock());
                               I != IE; ++I) {
    BasicBlock *BB = *I;
    if (BB->getName() != "START" && BB->getName() != "STOP") {
      cdg_outs << "\t" << BB->getName() << " [label=\"" << BB->getName() << "\", shape=circle];\n";
    }
  }

  cdg_outs << "\n";
  cdg_outs << "}\n";

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
