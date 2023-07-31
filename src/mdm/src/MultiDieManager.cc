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
#include <iostream>
#include "mdm/MultiDieManager.hh"

namespace mdm {

MultiDieManager::MultiDieManager() : param1_(0.0), flag1_(false)
{
}

MultiDieManager::~MultiDieManager()
{
}

void MultiDieManager::init(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
}

void MultiDieManager::setParam1(double param1)
{
  param1_ = param1;
}

void MultiDieManager::setFlag1(bool flag1)
{
  flag1_ = flag1;
}
void MultiDieManager::getFlag1()
{
  std::cout << flag1_ << std::endl;
}

}  // namespace mdm
