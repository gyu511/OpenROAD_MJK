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

void MultiDieManager::inheritRows(odb::dbBlock* parentBlock,
                                  odb::dbBlock* childBlock)
{
  for (auto row : parentBlock->getRows()) {
    int rowOriginX, rowOriginY;
    row->getOrigin(rowOriginX, rowOriginY);
    odb::dbRow::create(childBlock,
                       row->getName().c_str(),
                       row->getSite(),
                       rowOriginX,
                       rowOriginY,
                       row->getOrient(),
                       row->getDirection(),
                       row->getSiteCount(),
                       row->getSpacing());
  }
}

void MultiDieManager::constructSimpleExample1()
{
  numberOfDie_ = 2;

  int dieWidth = 1000;
  int dieHeight = 1000;
  odb::Point ll = odb::Point(0, 0);
  odb::Point ur = odb::Point(dieWidth, dieHeight);
  odb::Rect dieBox(ll, ur);

  // construct the db for the test
  auto chip = odb::dbChip::create(db_);
  auto tech = odb::dbTech::create(db_, "topHeirTech");
  auto topHeirBLock = odb::dbBlock::create(chip, "topHeirBlock", tech);
  topHeirBLock->setDieArea(dieBox);

  auto childTech1 = odb::dbTech::create(db_, "childTech1");
  auto childTech2 = odb::dbTech::create(db_, "childTech2");

  odb::dbTechLayer* techLayer1 = odb::dbTechLayer::create(
      childTech1, "layer", odb::dbTechLayerType::MASTERSLICE);
  odb::dbTechLayer* techLayer2 = odb::dbTechLayer::create(
      childTech2, "layer", odb::dbTechLayerType::MASTERSLICE);

  auto childBlock1 = odb::dbBlock::create(topHeirBLock, "block1", childTech1);
  auto childBlock2 = odb::dbBlock::create(topHeirBLock, "block2", childTech2);
  childBlock1->setDieArea(dieBox);
  childBlock2->setDieArea(dieBox);

  odb::dbInst::create(topHeirBLock, childBlock1, "bottomDie");
  odb::dbInst::create(topHeirBLock, childBlock2, "topDie");

  // make a master
  auto lib1 = odb::dbLib::create(db_, "lib1", childTech2);
  auto lib2 = odb::dbLib::create(db_, "lib2", childTech1);
  auto master1 = odb::dbMaster::create(lib1, "master1");
  auto master2 = odb::dbMaster::create(lib2, "master2");

  int cellWidth = 50;
  int cellHeight = 50;
  master1->setWidth(cellWidth);
  master2->setWidth(cellWidth);

  master1->setWidth(cellWidth);
  master2->setWidth(cellWidth);
  master1->setHeight(cellHeight);
  master2->setHeight(cellHeight);

  master1->setType(odb::dbMasterType::CORE);
  master2->setType(odb::dbMasterType::CORE);

  auto sigType = odb::dbSigType::SIGNAL;
  auto ioType = odb::dbIoType::INOUT;
  auto mTerm1 = odb::dbMTerm::create(master1, "mTerm1", ioType, sigType);
  auto mTerm2 = odb::dbMTerm::create(master2, "mTerm2", ioType, sigType);

  auto mPin1 = odb::dbMPin::create(mTerm1);
  auto mPin2 = odb::dbMPin::create(mTerm2);

  odb::dbBox::create(mPin1,
                     techLayer1,
                     cellWidth / 2,
                     cellHeight / 2,
                     cellWidth / 2 + 1,
                     cellHeight / 2 + 1);
  odb::dbBox::create(mPin2,
                     techLayer2,
                     cellWidth / 2,
                     cellHeight / 2,
                     cellWidth / 2 + 1,
                     cellHeight / 2 + 1);

  master1->setFrozen();
  master2->setFrozen();

  // make a instance
  auto inst1 = odb::dbInst::create(childBlock1, master1, "inst1");
  auto inst2 = odb::dbInst::create(childBlock2, master2, "inst2");

  // set the position of instances
  inst1->setLocation(ll.x() + cellWidth, ll.y() + cellHeight);
  inst2->setLocation(ur.x() - 2 * cellWidth, ur.y() - 2 * cellHeight);

  inst1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  inst2->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // make a net
  auto net1 = odb::dbNet::create(childBlock1, "net1");
  auto net2 = odb::dbNet::create(childBlock2, "net2");
  auto netTopHeir = odb::dbNet::create(topHeirBLock, "netTopHeir");

  // mark these nets are intersected net
  odb::dbBoolProperty::create(netTopHeir, "intersected", true);
  odb::dbBoolProperty::create(net1, "intersected", true);
  odb::dbBoolProperty::create(net2, "intersected", true);

  // connect the net to the instance
  (*inst1->getITerms().begin())->connect(net1);
  (*inst2->getITerms().begin())->connect(net2);

  // connect the intersected net between two dies
  auto bTerm1 = odb::dbBTerm::create(net1, "bTerm1");
  auto bTerm2 = odb::dbBTerm::create(net2, "bTerm2");

  bTerm1->getITerm()->connect(netTopHeir);
  bTerm2->getITerm()->connect(netTopHeir);

  // row construction
  auto site1 = odb::dbSite::create(lib1, "site1");
  auto site2 = odb::dbSite::create(lib2, "site2");
  auto site_width = cellWidth;
  auto site_height = cellHeight;
  site1->setWidth(site_width);
  site2->setWidth(site_width);
  site1->setHeight(site_height);
  site2->setHeight(site_height);

  auto numOfRow = dieHeight / site_height;
  auto numOfSite = dieWidth / site_width;
  for (int i = 0; i < numOfRow; ++i) {
    odb::dbRow::create(topHeirBLock,
                       ("row" + to_string(i)).c_str(),
                       site1,
                       0,
                       i * site_height,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite,
                       site_width);
    odb::dbRow::create(childBlock1,
                       ("row" + to_string(i)).c_str(),
                       site1,
                       0,
                       i * site_height,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite,
                       site_width);
    odb::dbRow::create(childBlock2,
                       ("row" + to_string(i)).c_str(),
                       site1,
                       0,
                       i * site_height,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite,
                       site_width);
  }

  logger_->info(utl::MDM, 2, "Construction simple db for GPL test");
}

void MultiDieManager::constructSimpleExample2()
{
  numberOfDie_ = 2;

  int dieWidth = 1000;
  int dieHeight = 1000;
  odb::Point ll = odb::Point(0, 0);
  odb::Point ur = odb::Point(dieWidth, dieHeight);
  odb::Rect dieBox(ll, ur);

  auto chip = odb::dbChip::create(db_);
  auto tech1 = odb::dbTech::create(db_, "tech1");
  auto tech2 = odb::dbTech::create(db_, "tech2");

  odb::dbTechLayer* techLayer1 = odb::dbTechLayer::create(
      tech1, "layer", odb::dbTechLayerType::MASTERSLICE);
  odb::dbTechLayer* techLayer2 = odb::dbTechLayer::create(
      tech2, "layer", odb::dbTechLayerType::MASTERSLICE);

  auto block1 = odb::dbBlock::create(chip, "block1", tech1);
  auto block2 = odb::dbBlock::create(block1, "block2", tech2);
  odb::dbInst::create(block1, block2, "interfaceInst");

  block1->setDieArea(dieBox);
  block2->setDieArea(dieBox);

  // make a master
  auto lib1 = odb::dbLib::create(db_, "lib1", tech1);
  auto lib2 = odb::dbLib::create(db_, "lib2", tech2);
  auto master1 = odb::dbMaster::create(lib1, "master1");
  auto master2 = odb::dbMaster::create(lib2, "master2");

  int cellWidth = 50;
  int cellHeight = 50;
  master1->setWidth(cellWidth);
  master2->setWidth(cellWidth);
  master1->setHeight(cellHeight);
  master2->setHeight(cellHeight);

  master1->setType(odb::dbMasterType::CORE);
  master2->setType(odb::dbMasterType::CORE);

  auto sigType = odb::dbSigType::SIGNAL;
  auto ioType = odb::dbIoType::INOUT;
  auto mTerm1 = odb::dbMTerm::create(master1, "mTerm1", ioType, sigType);
  auto mTerm2 = odb::dbMTerm::create(master2, "mTerm2", ioType, sigType);

  auto mPin1 = odb::dbMPin::create(mTerm1);
  auto mPin2 = odb::dbMPin::create(mTerm2);

  odb::dbBox::create(mPin1,
                     techLayer1,
                     cellWidth / 2,
                     cellHeight / 2,
                     cellWidth / 2 + 1,
                     cellHeight / 2 + 1);

  odb::dbBox::create(mPin2,
                     techLayer2,
                     cellWidth / 2,
                     cellHeight / 2,
                     cellWidth / 2 + 1,
                     cellHeight / 2 + 1);

  master1->setFrozen();
  master2->setFrozen();

  // make a instance
  auto inst1 = odb::dbInst::create(block1, master1, "inst1");
  auto inst2 = odb::dbInst::create(block2, master2, "inst2");

  // set the position of instances
  inst1->setLocation(ll.x() + cellWidth, ll.y() + cellHeight);
  inst2->setLocation(ur.x() - 2 * cellWidth, ur.y() - 2 * cellHeight);

  inst1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  inst2->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // make a net
  auto net1 = odb::dbNet::create(block1, "net1");
  auto net2 = odb::dbNet::create(block2, "net2");

  // mark these nets as intersected net
  odb::dbBoolProperty::create(net1, "intersected", true);
  odb::dbBoolProperty::create(net2, "intersected", true);

  // connect the net to the instance
  (*inst1->getITerms().begin())->connect(net1);
  (*inst2->getITerms().begin())->connect(net2);

  // connect the intersected nets
  auto bTerm = odb::dbBTerm::create(net2, "interfaceBTerm");
  bTerm->getITerm()->connect(net1);

  // row construction
  auto site1 = odb::dbSite::create(lib1, "site1");
  auto site2 = odb::dbSite::create(lib2, "site2");
  auto site_width = cellWidth;
  auto site_height = cellHeight;
  site1->setWidth(site_width);
  site2->setWidth(site_width);
  site1->setHeight(site_height);
  site2->setHeight(site_height);

  auto numOfRow = dieHeight / site_height;
  auto numOfSite = dieWidth / site_width;
  for (int i = 0; i < numOfRow; ++i) {
    auto rowName = "row" + to_string(i);
    odb::dbRow::create(block1,
                       rowName.c_str(),
                       site1,
                       0,
                       i * site_height,
                       odb::dbOrientType::R0,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite,
                       0);
    odb::dbRow::create(block2,
                       rowName.c_str(),
                       site2,
                       0,
                       i * site_height,
                       odb::dbOrientType::R0,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite,
                       0);
  }
}

}  // namespace mdm