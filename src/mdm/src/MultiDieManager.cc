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

#include <cstddef>
#include <iostream>

#include "utl/Logger.h"

namespace mdm {

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
                              uint hybrid_bond_space_y)
{
  hybrid_bond_info_.setHybridBondInfo(
      hybrid_bond_x, hybrid_bond_y, hybrid_bond_space_x, hybrid_bond_space_y);
  number_of_die_ = number_of_die;
  // partition_mgr_->set3DIC(number_of_die_);
  // replace_->set3DIC(number_of_die_);
  // opendp_->set3DIC(number_of_die_);
  logger_->info(utl::MDM, 1, "Set number of die to {}", number_of_die_);

  partitionInstances();
}
void MultiDieManager::partitionInstances()
{
  // read partition information
  // Here, let's do partitioning randomly, just for gpl testing.
  assert(number_of_die_ == 2);
  uint64_t area_sum = 0;
  for (auto inst : db_->getChip()->getBlock()->getInsts())
    area_sum += (static_cast<uint64_t>(inst->getBBox()->getDX()
                                       * inst->getBBox()->getDY()));

  uint64_t half_total_area = floor(area_sum / 2);
  area_sum = 0;
  auto block = db_->getChip()->getBlock();
  auto region1 = odb::dbRegion::create(block, "region1");
  auto region2 = odb::dbRegion::create(block, "region2");
  auto group1 = odb::dbGroup::create(region1, "top");
  auto group2 = odb::dbGroup::create(region2, "bottom");
  group1->setType(odb::dbGroupType::PHYSICAL_CLUSTER);
  group2->setType(odb::dbGroupType::PHYSICAL_CLUSTER);

  for (auto macro : db_->getChip()->getBlock()->getInsts()) {
    if (macro->isBlock()) {
      group1->addInst(macro);
      uint area = macro->getBBox()->getDX() * macro->getBBox()->getDY();
      area_sum += (static_cast<uint64_t>(area));
    }
  }

  for (auto inst : db_->getChip()->getBlock()->getInsts()) {
    if (inst->isBlock()) {
      continue;
    }
    uint area = inst->getBBox()->getDX() * inst->getBBox()->getDY();
    area_sum += (static_cast<uint64_t>(area));
    if (area_sum < half_total_area) {
      group1->addInst(inst);
    } else {
      group2->addInst(inst);
    }
  }
  logger_->info(utl::MDM, 2, "Mark Instances into Dies");
}

}  // namespace mdm
