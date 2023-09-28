///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mdm/MultiDieManager.hh"
#include "utl/Logger.h"
namespace mdm {
using namespace std;
void MultiDieManager::makeShrunkLefs()
{
  float shrinkLengthRatio = 1.0;
  for (int i = 0; i < numberOfDie_; ++i) {
    shrinkLengthRatios_.push_back(shrinkLengthRatio);
    if (i == 0) {
      continue;
    }
    string dieName = "Die" + to_string(i);
    shrinkLengthRatio = std::sqrt(shrinkAreaRatio_) * shrinkLengthRatio;
    auto tech = makeNewTech(dieName);
    makeShrunkLib(dieName, shrinkLengthRatio, tech);
  }
}

odb::dbTech* MultiDieManager::makeNewTech(const std::string& techName)
{
  auto tech = odb::dbTech::create(db_, (techName + "_tech").c_str());
  auto originalTech = (*db_->getTechs().begin());
  tech->setLefUnits(originalTech->getLefUnits());
  tech->setManufacturingGrid(originalTech->getManufacturingGrid());
  tech->setLefVersion(originalTech->getLefVersion());
  tech->setDbUnitsPerMicron(originalTech->getDbUnitsPerMicron());
  for (auto originalLayer : originalTech->getLayers()) {
    odb::dbTechLayer::create(tech,
                             (originalLayer->getName() + techName).c_str(),
                             originalLayer->getType());
  }
  return tech;
}

void MultiDieManager::makeShrunkLib(const string& whichDie,
                                    double shrunkRatio,
                                    odb::dbTech* tech)
{
  auto originalLibs = db_->getLibs();

  uint libNumber = originalLibs.size();
  uint libIdx = 0;
  for (auto originalLib : originalLibs) {
    libIdx++;
    if (libIdx > libNumber)  // NOLINT(*-braces-around-statements)
      break;

    auto lib = odb::dbLib::create(
        db_, (originalLib->getName() + whichDie).c_str(), tech);
    lib->setLefUnits(originalLib->getLefUnits());
    // create master when you can handle multi-die technology;
    // after revised odb version
    for (auto originalSite : originalLib->getSites()) {
      auto site = odb::dbSite::create(
          lib, (originalSite->getName() + whichDie).c_str());
      // apply shrink factor on site
      site->setWidth(static_cast<int>(
          static_cast<float>(originalSite->getWidth()) * shrunkRatio));
      site->setHeight(static_cast<int>(
          static_cast<float>(originalSite->getHeight()) * shrunkRatio));
    }

    for (auto originalMaster : originalLib->getMasters()) {
      odb::dbMaster* master
          = odb::dbMaster::create(lib, (originalMaster->getName()).c_str());

      master->setType(originalMaster->getType());
      if (originalMaster->getEEQ())
        master->setEEQ(originalMaster->getEEQ());
      if (originalMaster->getLEQ())
        master->setLEQ(originalMaster->getLEQ());
      int width = static_cast<int>(
          static_cast<float>(originalMaster->getWidth()) * shrunkRatio);
      int height = static_cast<int>(
          static_cast<float>(originalMaster->getHeight()) * shrunkRatio);
      master->setWidth(width);
      master->setHeight(height);

      int masterOriginX, masterOriginY;
      originalMaster->getOrigin(masterOriginX, masterOriginY);
      masterOriginX
          = static_cast<int>(static_cast<float>(masterOriginX) * shrunkRatio);
      masterOriginY
          = static_cast<int>(static_cast<float>(masterOriginY) * shrunkRatio);
      master->setOrigin(masterOriginX, masterOriginY);

      if (originalMaster->getSite() != nullptr) {
        auto site = lib->findSite(
            (originalMaster->getSite()->getName() + whichDie).c_str());
        assert(site != nullptr);
        master->setSite(site);
      }

      if (originalMaster->getSymmetryX())
        master->setSymmetryX();
      if (originalMaster->getSymmetryY())
        master->setSymmetryY();
      if (originalMaster->getSymmetryR90())
        master->setSymmetryR90();

      master->setMark(originalMaster->isMarked());
      master->setSequential(originalMaster->isSequential());
      master->setSpecialPower(originalMaster->isSpecialPower());

      for (auto originalMTerm : originalMaster->getMTerms()) {
        auto dbMTerm = odb::dbMTerm::create(master,
                                            originalMTerm->getConstName(),
                                            originalMTerm->getIoType(),
                                            originalMTerm->getSigType(),
                                            originalMTerm->getShape());

        for (auto pin_original : originalMTerm->getMPins()) {
          auto dbMPin = odb::dbMPin::create(dbMTerm);
          for (auto geometry : pin_original->getGeometry()) {
            int x1, y1, x2, y2;
            x1 = static_cast<int>(static_cast<float>(geometry->xMin())
                                  * shrunkRatio);
            y1 = static_cast<int>(static_cast<float>(geometry->yMin())
                                  * shrunkRatio);
            x2 = static_cast<int>(static_cast<float>(geometry->xMax())
                                  * shrunkRatio);
            y2 = static_cast<int>(static_cast<float>(geometry->yMax())
                                  * shrunkRatio);
            odb::dbBox::create(
                dbMPin, geometry->getTechLayer(), x1, y1, x2, y2);
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
  for (auto libIter : db_->getLibs()) {
    if (iter == value) {
      return libIter;
    }
    iter++;
  }
  return nullptr;  // or handle appropriately if lib is not found
}

void MultiDieManager::readPartitionInfo(const std::string& fileName)
{
  // read partition file and apply it
  ifstream partitionFile(fileName);
  if (!partitionFile.is_open()) {
    logger_->error(utl::MDM, 4, "Cannot open partition file");
  }
  string line;
  while (getline(partitionFile, line)) {
    istringstream iss(line);
    string instName;
    int partitionId;
    iss >> instName >> partitionId;
    odb::dbInst* inst;
    for (auto chip : db_->getChips()) {
      inst = chip->getBlock()->findInst(instName.c_str());
      if (!inst) {
        for (auto block : chip->getBlock()->getChildren()) {
          inst = block->findInst(instName.c_str());
          if (inst) {
            continue;
          }
        }
      }
    }
    if (inst != nullptr) {
      odb::dbIntProperty::create(inst, "partition_id", partitionId);
    }
  }
  partitionFile.close();
}

}  // namespace mdm