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

void MultiDieManager::makeShrunkLefs()
{
  float shrink_length_ratio = 1.0;
  for (int i = 0; i < number_of_die_; ++i) {
    shrink_length_ratios_.push_back(shrink_length_ratio);
    if (i == 0) {
      continue;
    }
    string die_name = "Die" + to_string(i);
    shrink_length_ratio = std::sqrt(shrink_area_ratio) * shrink_length_ratio;
    auto tech = makeNewTech(die_name);
    makeShrunkLib(die_name, shrink_length_ratio, tech);
  }
}

odb::dbTech* MultiDieManager::makeNewTech(const std::string& tech_name)
{
  auto tech = odb::dbTech::create(db_, (tech_name + "_tech").c_str());
  auto tech_original = (*db_->getTechs().begin());
  tech->setLefUnits(tech_original->getLefUnits());
  tech->setManufacturingGrid(tech_original->getManufacturingGrid());
  tech->setLefVersion(tech_original->getLefVersion());
  tech->setDbUnitsPerMicron(tech_original->getDbUnitsPerMicron());
  for (auto layer_original : tech_original->getLayers()) {
    odb::dbTechLayer::create(tech,
                             (layer_original->getName() + tech_name).c_str(),
                             layer_original->getType());
  }
  return tech;
}

void MultiDieManager::makeShrunkLib(const string& which_die,
                                    double shrunk_ratio,
                                    odb::dbTech* tech)
{
  auto libs_original = db_->getLibs();

  uint lib_number = libs_original.size();
  uint lib_idx = 0;
  for (auto lib_original : libs_original) {
    lib_idx++;
    if (lib_idx > lib_number)  // NOLINT(*-braces-around-statements)
      break;

    auto lib = odb::dbLib::create(
        db_, (lib_original->getName() + which_die).c_str(), tech);
    lib->setLefUnits(lib_original->getLefUnits());
    // create master when you can handle multi-die technology;
    // after revised odb version
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

      if (master_original->getSite() != nullptr) {
        auto site = lib->findSite(
            (master_original->getSite()->getName() + which_die).c_str());
        assert(site != nullptr);
        master->setSite(site);
      }

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


// return the n_th lib
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

void MultiDieManager::readPartitionInfo(std::string file_name)
{
  // read partition file and apply it
  ifstream partition_file(file_name);
  if (!partition_file.is_open()) {
    logger_->error(utl::MDM, 4, "Cannot open partition file");
  }
  string line;
  while (getline(partition_file, line)) {
    istringstream iss(line);
    string inst_name;
    int partition_id;
    iss >> inst_name >> partition_id;
    odb::dbInst* inst;
    for (auto chip : db_->getChips()) {
      inst = chip->getBlock()->findInst(inst_name.c_str());
    }
    if (inst != nullptr) {
      odb::dbIntProperty::create(inst, "partition_id", partition_id);
    }
  }
  partition_file.close();
}

}  // namespace mdm
