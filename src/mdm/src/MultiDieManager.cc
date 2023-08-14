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
  // check the partition information exists and apply the information at the
  // same time
  for (auto chip : db_->getChips()) {
    auto block = chip->getBlock();
    for (auto inst : block->getInsts()) {
      auto partition_info = odb::dbIntProperty::find(inst, "partition_id");
      if (partition_info) {
        int partition_info_int = partition_info->getValue();
        string partition_info_str = "Die" + to_string(partition_info_int);
        odb::dbIntProperty::create(inst, "which_die", partition_info_int);

        auto group = odb::dbGroup::create(block, partition_info_str.c_str());
        if (group == nullptr) {
          group = odb::dbGroup::getGroup(block, partition_info_int);
        }
        group->addInst(inst);
      } else {
        logger_->error(utl::MDM,
                       2,
                       "Do partition first. There are some instances that "
                       "don't have a partition info");
      }
    }
  }
  logger_->info(utl::MDM, 3, "Partition information is applied to instances");
}

}  // namespace mdm
