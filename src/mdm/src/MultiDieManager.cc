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
                              uint hybrid_bond_space_y,
                              float area_ratio)
{
  logger_->info(utl::MDM, 1, "Set number of die to {}", number_of_die_);

  // set the variables
  hybrid_bond_info_.setHybridBondInfo(
      hybrid_bond_x, hybrid_bond_y, hybrid_bond_space_x, hybrid_bond_space_y);
  number_of_die_ = number_of_die;
  shrink_area_ratio = area_ratio;

  // Set up for multi dies
  setUp();
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
  // check whether the partition information exists and
  // apply the information at the same time
  vector<odb::dbGroup*> groups;

  for (auto chip : db_->getChips()) {
    auto block = chip->getBlock();
    for (auto inst : block->getInsts()) {
      auto partition_info = odb::dbIntProperty::find(inst, "partition_id");
      if (partition_info) {
        int partition_info_int = partition_info->getValue();
        string partition_info_str = "Die" + to_string(partition_info_int);
        odb::dbIntProperty::create(inst, "which_die", partition_info_int);

        auto region = odb::dbRegion::create(block, partition_info_str.c_str());
        odb::dbGroup* group;
        if (region != nullptr) {
          group = odb::dbGroup::create(region, partition_info_str.c_str());
          groups.push_back(group);
        } else {
          group = groups.at(partition_info_int);
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
}

void MultiDieManager::switchMasters()
{
  for (auto chip : db_->getChips()) {
    auto block = chip->getBlock();
    for (auto inst : block->getInsts()) {
      auto partition_info = odb::dbIntProperty::find(inst, "which_die");
      assert(partition_info != nullptr);

      odb::dbLib* lib = findLibByPartitionInfo(partition_info->getValue());
      assert(lib);

      auto master = lib->findMaster(inst->getMaster()->getName().c_str());
      assert(master);

      switchMaster(inst, master);
    }
  }
  logger_->info(utl::MDM, 3, "Partition information is applied to instances");
}
void MultiDieManager::switchMaster(odb::dbInst* inst, odb::dbMaster* master)
{
  assert(inst->getMaster()->getName() == master->getName());
  assert(inst->getMaster()->getMTermCount() == master->getMTermCount());

  vector<pair<string, odb::dbNet*>> net_info_container;
  string inst_name = inst->getName();
  auto block = inst->getBlock();  // TODO: edit here after revised odb

  // save placement information
  odb::dbPlacementStatus placement_status = inst->getPlacementStatus();
  odb::Point location;
  if (placement_status != odb::dbPlacementStatus::NONE) {
    location = inst->getLocation();
  }

  // save connected net information
  for (auto iTerm : inst->getITerms()) {
    if (iTerm->getNet() == nullptr) {
      continue;
    }
    string pin_name = iTerm->getMTerm()->getName();
    net_info_container.emplace_back(pin_name, iTerm->getNet());
  }

  // save 4 kinds of properties
  struct PropertyStorage
  {
    vector<int> int_properties;
    vector<string> string_properties;
    vector<double> double_properties;
    vector<bool> bool_properties;
    vector<string> name_of_int_properties;
    vector<string> name_of_string_properties;
    vector<string> name_of_double_properties;
    vector<string> name_of_bool_properties;
  };
  PropertyStorage property_storage;
  for (auto property : odb::dbProperty::getProperties(inst)) {
    if (property->getType() == odb::dbProperty::Type::INT_PROP) {
      property_storage.int_properties.push_back(
          odb::dbIntProperty::find(inst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_int_properties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::Type::STRING_PROP) {
      property_storage.string_properties.push_back(
          odb::dbStringProperty::find(inst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_string_properties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::Type::DOUBLE_PROP) {
      property_storage.double_properties.push_back(
          odb::dbDoubleProperty::find(inst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_double_properties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::Type::BOOL_PROP) {
      property_storage.bool_properties.push_back(
          odb::dbBoolProperty::find(inst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_bool_properties.push_back(property->getName());
    }
  }
  // destory the instance
  odb::dbInst::destroy(inst);

  // remake the instance with new master
  inst = odb::dbInst::create(block, master, inst_name.c_str());

  // set placement information
  inst->setPlacementStatus(placement_status);
  if (placement_status) {
    inst->setLocation(location.getX(), location.getY());
  }

  // set connected net information
  for (const auto& net_info : net_info_container) {
    auto iterm = inst->findITerm(net_info.first.c_str());
    assert(iterm);
    iterm->connect(net_info.second);
  }

  // set 4 kinds of properties
  for (int i = 0; i < property_storage.int_properties.size(); ++i) {
    odb::dbIntProperty::create(
        inst,
        property_storage.name_of_int_properties.at(i).c_str(),
        property_storage.int_properties.at(i));
  }
  for (int i = 0; i < property_storage.string_properties.size(); ++i) {
    odb::dbStringProperty::create(
        inst,
        property_storage.name_of_string_properties.at(i).c_str(),
        property_storage.string_properties.at(i).c_str());
  }
  for (int i = 0; i < property_storage.double_properties.size(); ++i) {
    odb::dbDoubleProperty::create(
        inst,
        property_storage.name_of_double_properties.at(i).c_str(),
        property_storage.double_properties.at(i));
  }
  for (int i = 0; i < property_storage.bool_properties.size(); ++i) {
    odb::dbBoolProperty::create(
        inst,
        property_storage.name_of_bool_properties.at(i).c_str(),
        property_storage.bool_properties.at(i));
  }
}
odb::dbLib* MultiDieManager::findLibByPartitionInfo(int value)
{
  int iter = 0;
  for (auto lib_iter : db_->getLibs()) {
    if (iter == value) {
      return lib_iter;
    }
    iter++;
  }
  return nullptr;  // or handle appropriately if lib is not found
}

}  // namespace mdm
