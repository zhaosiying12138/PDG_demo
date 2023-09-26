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

    bool operator ==(const CDGNode &other)
    {
      return (type == other.type) && (BB == other.BB) && (condition == other.condition) && (type == 0);
    }
  };
  struct RegionNode {
    int id;
    std::list<CDGNode> data;
  };
  int total_region_node_id = 1;

  std::vector<RegionNode *> region_record;
  std::map<BasicBlock *, RegionNode *> map_bb_region;

  void zsy_dump_region_node_data(std::list<CDGNode> data)
  {
    for (CDGNode cdg_node : data) {
      if (cdg_node.type == 0) {
        llvm::outs() << cdg_node.BB->getName() << "-";
        if (cdg_node.condition)
          llvm::outs() << "T";
        else
          llvm::outs() << "F";
        llvm::outs() << ", ";
      } else {
        llvm::outs() << "R" << cdg_node.RN->id << ", ";
      }
    }
    llvm::outs() << "\n";
  }

  int cdg_region_node_basic_block_size(RegionNode *rn)
  {
    int cnt = 0;
    for (CDGNode cdg_node : rn->data) {
      if (cdg_node.type == 0)
        cnt++;
    }
    return cnt;
  }

  bool cdg_is_region_node_contains_data(RegionNode *rn, std::list<CDGNode> data)
  {
    for (auto it1 = data.begin(); it1 != data.end(); it1++) {
      CDGNode cdg_node1 = *it1;
      bool immediate_ret = false;
      for (auto it2 = rn->data.begin(); it2 != rn->data.end(); it2++) {
        CDGNode cdg_node2 = *it2;
        if (cdg_node1.type == cdg_node2.type && cdg_node1.BB == cdg_node2.BB
          && cdg_node1.condition == cdg_node2.condition) {
            immediate_ret = true;
            break;
        }
      }
      if(!immediate_ret)
        return false;
    }
    return true;
  }

  RegionNode *cdg_is_equal_region_node(std::list<CDGNode> data)
  {
    // llvm::outs() << "[ZSY_DBG] cdg_is_equal_region_node: ";
    // zsy_dump_region_node_data(data);
    for (int i = 0; i < region_record.size(); i++) {
      RegionNode *rn = region_record.at(i);
      // llvm::outs() << "[ZSY_DBGG] to be test from: \n";
      // zsy_dump_region_node(rn);
      int bb_cnt = cdg_region_node_basic_block_size(rn);
      if (bb_cnt != data.size()) {
        continue;
      }

      if (cdg_is_region_node_contains_data(rn, data)) {        
        return rn;
      }
    }
    return NULL;
  }

  RegionNode *cdg_is_subset_region_node(std::list<CDGNode> data)
  {
    for (int i = 0; i < region_record.size(); i++) {
      RegionNode *rn = region_record.at(i);
      if (cdg_is_region_node_contains_data(rn, data)) {        
        return rn;
      }
    }
    return NULL;
  }

  RegionNode *cdg_alloc_region_node(std::list<CDGNode> data)
  {
    RegionNode *new_region_node = new RegionNode();
    new_region_node->id = total_region_node_id++;
    new_region_node->data = data;
    return new_region_node;
  }

  void cdg_remove_containing_region_from_select_data(RegionNode *containing_rn, std::list<CDGNode> data)
  {
    for (CDGNode cdg_remove_node : data) {
      containing_rn->data.remove(cdg_remove_node);
    }
  }

  void cdg_replace_containing_region_with_contained(RegionNode *containing_rn, RegionNode *contained_rn)
  {
    cdg_remove_containing_region_from_select_data(containing_rn, contained_rn->data);

    CDGNode new_cdg_node;
    new_cdg_node.type = 1;
    new_cdg_node.RN = contained_rn;
    containing_rn->data.push_back(new_cdg_node);
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
        RegionNode *cdg_containing_region = cdg_is_subset_region_node(tmp_cdg_region_data);
        if (cdg_containing_region != NULL) {
          cdg_replace_containing_region_with_contained(cdg_containing_region, cdg_reuse_region);
        }
        region_record.push_back(cdg_reuse_region);
      }
      map_bb_region.emplace(BB, cdg_reuse_region);
      
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
