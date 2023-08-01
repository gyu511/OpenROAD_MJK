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

#include <iostream>

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

void MultiDieManager::setDieNum(int number_of_die)
{
  number_of_die_ = number_of_die;
  // partition_mgr_->setDieNum(number_of_die_);
  // replace_->setDieNum(number_of_die_);
  // opendp_->setDieNum(number_of_die_);
  logger_->info(utl::MDM, 1, "Set number of die to {}", number_of_die_);
}
void MultiDieManager::partitionInstances()
{
}

}  // namespace mdm
