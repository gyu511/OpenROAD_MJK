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
      if (!inst) {
        for (auto block : chip->getBlock()->getChildren()) {
          inst = block->findInst(inst_name.c_str());
          if (inst) {
            continue;
          }
        }
      }
    }
    if (inst != nullptr) {
      odb::dbIntProperty::create(inst, "partition_id", partition_id);
    }
  }
  partition_file.close();
}

}  // namespace mdm