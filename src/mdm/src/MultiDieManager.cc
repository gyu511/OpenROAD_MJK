///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mdm/MultiDieManager.hh"

#include "utl/Logger.h"

namespace mdm {
using namespace std;

MultiDieManager::MultiDieManager() = default;

MultiDieManager::~MultiDieManager() = default;

void MultiDieManager::init(odb::dbDatabase* db,
                           utl::Logger* logger,
                           par::PartitionMgr* partitionMgr,
                           gpl::Replace* replace,
                           dpl::Opendp* opendp)
{
  db_ = db;
  logger_ = logger;
  partitionMgr_ = partitionMgr;
  replace_ = replace;
  opendp_ = opendp;
}

void MultiDieManager::set3DIC(int numberOfDie,
                              uint hybridBondX,
                              uint hybridBondY,
                              uint hybridBondSpaceX,
                              uint hybridBondSpaceY,
                              float areaRatio)
{
  // set the variables
  hybridBondInfo_.setHybridBondInfo(
      hybridBondX, hybridBondY, hybridBondSpaceX, hybridBondSpaceY);
  numberOfDie_ = numberOfDie;
  shrinkAreaRatio_ = areaRatio;

  logger_->info(utl::MDM, 1, "Set number of die to {}", numberOfDie_);
  // Set up for multi dies
  splitInstances();
}

void MultiDieManager::splitInstances()
{
  // TODO: We need to make the partitioning part be in the TCL command.
  readPartitionInfo("partitionInfo/ispd18_test1.par");
  makeSubBlocks();

  // for the most bottom die, the instances is already assigned in the tcl
  // level. e.g. read_def -child -tech bottom
  // So we need switch instances from bottom to other dies
  auto mostBottomDie = *db_->getChip()->getBlock()->getChildren().begin();
  for (auto inst : mostBottomDie->getInsts()) {
    // TODO: check the iterator is fine even though the destroy of the insets
    switchInstanceToAssignedDie(inst);
  }

  // make the interconnections between (m) and (m+1)th blocks
  auto blocks = db_->getChip()->getBlock()->getChildren();
  auto iter = blocks.begin();
  odb::dbBlock* lowerBlock = *iter;
  for (++iter; iter != blocks.end(); ++iter) {
    odb::dbBlock* upperBlock = *iter;
    makeInterconnections(lowerBlock, upperBlock);
    lowerBlock = upperBlock;
  }
}
void MultiDieManager::makeSubBlocks()
{
  auto topHeirBlock = db_->getChip()->getBlock();
  // +1 for the top heir block
  assert(numberOfDie_ + 1 == db_->getTechs().size());
  assert(db_->getTechs().size() >= 2);

  int dieIdx = 0;
  for (auto tech : db_->getTechs()) {
    if (tech == *db_->getTechs().begin()) {
      // exclude the first tech which is the top heir,
      continue;
    }
    string dieName = "Die" + to_string(dieIdx);
    odb::dbBlock* childBlock = nullptr;
    if (dieIdx != 0) {
      // the second which is parsed at the tcl level
      // e.g. read_def -child -tech top ispd18_test1.input.def
      childBlock = odb::dbBlock::create(topHeirBlock, dieName.c_str(), tech);
    } else {
      childBlock = (*db_->getChip()->getBlock()->getChildren().begin());
      // TODO: @Matt ,
      //  this is minor, but I want to change the `childBlock` name,
      //  which is parsed by the tcl level. Is this possible?
      //  Currently, the name of
      //  child_block_0 = ispd18~, child_block1 = Die1, child_block2 = Die2...
    }
    // make the inst that can represent the child die
    odb::dbInst::create(topHeirBlock, childBlock, dieName.c_str());
    dieIdx++;
  }
}
void MultiDieManager::switchInstanceToAssignedDie(odb::dbInst* originalInst)
{
  auto targetBlockID = SwitchInstanceHelper::findAssignedDieId(originalInst);
  if (targetBlockID == 0) {
    return;  // No need to switch if it is on the first die
  }
  auto [targetBlock, targetLib]
      = SwitchInstanceHelper::findTargetDieAndLib(this, targetBlockID);
  auto targetMaster
      = targetLib->findMaster(originalInst->getMaster()->getName().c_str());

  auto newInst = odb::dbInst::create(
      targetBlock, targetMaster, originalInst->getName().c_str());
  SwitchInstanceHelper::inheritPlacementInfo(originalInst, newInst);
  SwitchInstanceHelper::inheritNetInfo(originalInst, newInst);
  SwitchInstanceHelper::inheritProperties(originalInst, newInst);
  SwitchInstanceHelper::inheritGroupInfo(originalInst, newInst);

  odb::dbInst::destroy(originalInst);
}

void MultiDieManager::makeInterconnections(odb::dbBlock* lowerBlock,
                                           odb::dbBlock* upperBlock)
{
  // interconnection should be implemented through
  // lowerBlock - top heir block - upperBlock

  // traverse the nets
  for (auto lowerBlockNet : lowerBlock->getNets()) {
    auto netName = lowerBlockNet->getName();
    auto upperBlockNet = upperBlock->findNet(netName.c_str());
    if (upperBlockNet) {
      // access or make the interconnected net at top heir block
      auto topHeirBlock = db_->getChip()->getBlock();
      auto topHeirNet = topHeirBlock->findNet(netName.c_str());
      if (!topHeirNet) {
        topHeirNet = odb::dbNet::create(topHeirBlock, netName.c_str());
      }
      // connect between topHeir - lowerBlock, and between topHeir - upperBlock
      auto interconnectName = netName + "_interconnect";
      auto lowerBlockTerm
          = odb::dbBTerm::create(lowerBlockNet, interconnectName.c_str());
      auto upperBlockTerm
          = odb::dbBTerm::create(upperBlockNet, interconnectName.c_str());

      // REMINDER
      // Current State (only for the lower die):
      // 1. block terminal in lower block (`lowerBlockTerm`)
      // is connected with `instForLowerBlock`
      // 2. Let the `instForLowerBlock` be
      // the instance which is in the top heir block and is for lower block
      // Then iterm of `lowerInst` is not connected to any net

      auto instForLowerBlock = lowerBlock->getParentInst();
      auto instForUpperBlock = upperBlock->getParentInst();

      instForLowerBlock->findITerm(interconnectName.c_str())
          ->connect(topHeirNet);
      instForUpperBlock->findITerm(interconnectName.c_str())
          ->connect(topHeirNet);

      // @Matt, I have a question. What is the different  between
      // lowerBlock->getITerms() and instForLowerBlock->getITerms()?
      // This is different, right?
    }
  }
}

}  // namespace mdm
