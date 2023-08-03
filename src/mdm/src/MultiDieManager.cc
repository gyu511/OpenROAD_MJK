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

#include <cmath>
#include <iostream>

#include "utl/Logger.h"

namespace mdm {

MultiDieManager::MultiDieManager()
{
}

MultiDieManager::~MultiDieManager()
{
}

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

  // Set up for multi dies
  setUp();

  // replace_->set3DIC(number_of_die_);
  // opendp_->set3DIC(number_of_die_);
  logger_->info(utl::MDM, 1, "Set number of die to {}", number_of_die_);
}
void MultiDieManager::setUp()
{
  makeShrunkLefs();
  partitionInstances();
  switchMasters();
}

void MultiDieManager::makeShrunkLefs()
{
  // set `shrink_length_ratios_`
  auto db_database_original = db_;
  const std::string& lef_path = "../../src/mdm/test/shrunkLefs/";

  float shrink_length_ratio = 1.0;
  for (int i = 0; i < number_of_die_; ++i) {
    shrink_length_ratios_.push_back(shrink_length_ratio);
    shrink_length_ratio = std::sqrt(shrink_length_ratio);
  }
}
void MultiDieManager::makeShrunkLef()
{
}
void MultiDieManager::readShrunkLibs()
{
}
void MultiDieManager::partitionInstances()
{
}
void MultiDieManager::switchMasters()
{
}

}  // namespace mdm
