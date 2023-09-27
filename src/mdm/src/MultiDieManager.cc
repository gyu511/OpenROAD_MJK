// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "mdm/MultiDieManager.hh"

#include "utl/Logger.h"

namespace mdm {
using namespace std;

MultiDieManager::MultiDieManager() = default;

MultiDieManager::~MultiDieManager() = default;

void MultiDieManager::init(odb::dbDatabase* db,
                           utl::Logger* logger,
                           par::PartitionMgr* partition_mgr,
                           gpl::Replace* replace,
                           dpl::Opendp* opendp)
{
  db_ = db;
  logger_ = logger;
  partition_mgr_ = partition_mgr;
  replace_ = replace;
  opendp_ = opendp;
}

void MultiDieManager::set3DIC(int number_of_die,
                              uint hybrid_bond_x,
                              uint hybrid_bond_y,
                              uint hybrid_bond_space_x,
                              uint hybrid_bond_space_y,
                              float area_ratio)
{
  // set the variables
  hybrid_bond_info_.setHybridBondInfo(
      hybrid_bond_x, hybrid_bond_y, hybrid_bond_space_x, hybrid_bond_space_y);
  number_of_die_ = number_of_die;
  shrink_area_ratio = area_ratio;

  logger_->info(utl::MDM, 1, "Set number of die to {}", number_of_die_);
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
  auto most_bottom_die = *db_->getChip()->getBlock()->getChildren().begin();
  for (auto inst : most_bottom_die->getInsts()) {
    switchInstanceToAssignedDie(inst);
  }
}
void MultiDieManager::makeSubBlocks()
{
  auto top_block = db_->getChip()->getBlock();
  // +1 for the top heir block
  assert(number_of_die_ + 1 == db_->getTechs().size());
  assert(db_->getTechs().size() >= 2);

  int die_idx = 0;
  for (auto tech : db_->getTechs()) {
    if (tech == *db_->getTechs().begin()) {
      // exclude the first tech which is the top heir,
      continue;
    }
    string die_name = "Die" + to_string(die_idx);
    odb::dbBlock* child_block = nullptr;
    if (die_idx != 0) {
      // the second which is parsed at the tcl level
      // e.g. read_def -child -tech top ispd18_test1.input.def
      child_block = odb::dbBlock::create(top_block, die_name.c_str(), tech);
    } else {
      child_block = (*db_->getChip()->getBlock()->getChildren().begin());
      // TODO: @Matt ,
      //  this is minor, but I want to change the `child_block` name,
      //  which is parsed by the tcl level. Is this possible?
      //  Currently, the name of
      //  child_block_0 = ispd18~, child_block1 = Die1, child_block2 = Die2...
    }
    // make the inst that can represent the child die
    odb::dbInst::create(top_block, child_block, die_name.c_str());
    die_idx++;
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

}  // namespace mdm
