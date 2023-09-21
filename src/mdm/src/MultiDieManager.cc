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
  setUp();
}
void MultiDieManager::setUp()
{
  splitInstances();
}

void MultiDieManager::splitInstances()
{
  // move the instances from top heir block to sub-block
  readPartitionInfo("partitionInfo/ispd18_test1.par"); // TODO: we need to this part make be in the tcl command
  switchMasters();
}

void MultiDieManager::switchMasters()
{
  for (auto chip : db_->getChips()) {
    auto block = chip->getBlock();
    for (auto inst : block->getInsts()) {
      auto partition_info = odb::dbIntProperty::find(inst, "partition_id");
      assert(partition_info != nullptr);

      int lib_order = partition_info->getValue();
      odb::dbLib* lib = findLibByPartitionInfo(lib_order); // return the n_th lib
      assert(lib);

      auto master = lib->findMaster(inst->getMaster()->getName().c_str());
      assert(master);

      switchMaster(inst, master, lib_order);
    }
  }
  logger_->info(utl::MDM, 3, "Partition information is applied to instances");
}

void MultiDieManager::switchMaster(odb::dbInst* inst,
                                   odb::dbMaster* master,
                                   int lib_order)
{
  assert(inst->getMaster()->getName() == master->getName());
  assert(inst->getMaster()->getMTermCount() == master->getMTermCount());

  vector<pair<string, odb::dbNet*>> net_info_container;
  string inst_name = inst->getName();

  // TODO: Question. How can I find the block correspond to the lib that I want,
  //  from lib? ( e.g. master->getLib()->...->getBlock() )
  auto block = db_->getChip()->getBlock()->findChild(("Die"+ to_string(lib_order)).c_str());
  assert(block);

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

  // save the group information
  odb::dbGroup* group = inst->getGroup();

  // destory the instance
  odb::dbInst::destroy(inst);

  // remake the instance with new master
  inst = odb::dbInst::create(block, master, inst_name.c_str());

  // set placement information
  if (placement_status) {
    inst->setLocation(location.getX(), location.getY());
  }
  inst->setPlacementStatus(placement_status);

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

  // set the group
  group->addInst(inst);
}



}  // namespace mdm
