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
  float shrink_length_ratio = 1.0;
  for (int i = 0; i < number_of_die_; ++i) {
    shrink_length_ratios_.push_back(shrink_length_ratio);
    shrink_length_ratio = std::sqrt(shrink_length_ratio);
    if (i == 0) {
      continue;
    }
    string die_name = "Die" + to_string(i);
    makeShrunkLef(die_name, shrink_length_ratio);
  }
}
void MultiDieManager::makeShrunkLef(const string& which_die,
                                    double shrunk_ratio)
{
  auto libs_original = db_->getLibs();
  auto tech = db_->getTech();

  // TODO:
  // Making multiple Technology should be implemented after the updated odb

  uint lib_number = libs_original.size();
  uint lib_idx = 0;
  for (auto lib_original : libs_original) {
    lib_idx++;
    if (lib_idx > lib_number)  // NOLINT(*-braces-around-statements)
      break;

    auto lib = odb::dbLib::create(
        db_, (lib_original->getName() + which_die).c_str(), tech);
    lib->setLefUnits(lib_original->getLefUnits());
    for (auto site_original : lib_original->getSites()) {
      auto site = odb::dbSite::create(
          lib, (site_original->getName() + which_die).c_str());
      // apply shrink factor on site
      site->setWidth(static_cast<int>(
          static_cast<float>(site_original->getWidth()) * shrunk_ratio));
      site->setHeight(static_cast<int>(
          static_cast<float>(site_original->getHeight()) * shrunk_ratio));
    }

    for (auto master_original : lib_original->getMasters()) {
      odb::dbMaster* master
          = odb::dbMaster::create(lib, (master_original->getName()).c_str());

      master->setType(master_original->getType());
      if (master_original->getEEQ())
        master->setEEQ(master_original->getEEQ());
      if (master_original->getLEQ())
        master->setLEQ(master_original->getLEQ());
      int width = static_cast<int>(
          static_cast<float>(master_original->getWidth()) * shrunk_ratio);
      int height = static_cast<int>(
          static_cast<float>(master_original->getHeight()) * shrunk_ratio);
      master->setWidth(width);
      master->setHeight(height);

      int master_origin_x, master_origin_y;
      master_original->getOrigin(master_origin_x, master_origin_y);
      master_origin_x = static_cast<int>(static_cast<float>(master_origin_x)
                                         * shrunk_ratio);
      master_origin_y = static_cast<int>(static_cast<float>(master_origin_y)
                                         * shrunk_ratio);
      master->setOrigin(master_origin_x, master_origin_y);

      std::string site_name = master_original->getSite()->getName() + which_die;
      auto site = lib->findSite(site_name.c_str());
      master->setSite(site);

      if (master_original->getSymmetryX())
        master->setSymmetryX();
      if (master_original->getSymmetryY())
        master->setSymmetryY();
      if (master_original->getSymmetryR90())
        master->setSymmetryR90();

      master->setMark(master_original->isMarked());
      master->setSequential(master_original->isSequential());
      master->setSpecialPower(master_original->isSpecialPower());

      for (auto m_term_original : master_original->getMTerms()) {
        auto db_m_term = odb::dbMTerm::create(master,
                                              m_term_original->getConstName(),
                                              m_term_original->getIoType(),
                                              m_term_original->getSigType(),
                                              m_term_original->getShape());

        for (auto pin_original : m_term_original->getMPins()) {
          auto db_m_pin = odb::dbMPin::create(db_m_term);
          for (auto geometry : pin_original->getGeometry()) {
            int x1, y1, x2, y2;
            x1 = static_cast<int>(static_cast<float>(geometry->xMin())
                                  * shrunk_ratio);
            y1 = static_cast<int>(static_cast<float>(geometry->yMin())
                                  * shrunk_ratio);
            x2 = static_cast<int>(static_cast<float>(geometry->xMax())
                                  * shrunk_ratio);
            y2 = static_cast<int>(static_cast<float>(geometry->yMax())
                                  * shrunk_ratio);
            odb::dbBox::create(
                db_m_pin, geometry->getTechLayer(), x1, y1, x2, y2);
          }
        }
      }
      master->setFrozen();
    }
  }
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
void MultiDieManager::switchMasters()
{
}
void MultiDieManager::switchMaster(odb::dbInst* inst, odb::dbMaster* master)
{
}

}  // namespace mdm
