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

void MultiDieManager::timingTest1()
{
  numberOfDie_ = 2;
  auto dbUnit = db_->getChip()->getBlock()->getDbUnitsPerMicron();
  int dieWidth = 1000 * dbUnit;
  int dieHeight = 1000 * dbUnit;
  odb::Point ll = odb::Point(0, 0);
  odb::Point ur = odb::Point(dieWidth, dieHeight);
  odb::Rect dieBox(ll, ur);

  auto topHeirBlock = db_->getChip()->getBlock();
  topHeirBlock->setDieArea(dieBox);

  auto techLayerTopHier = odb::dbTechLayer::create(
      db_->findTech("topHierTech"), "M2", odb::dbTechLayerType::ROUTING);

  auto lib1 = db_->findLib("example1-1");
  auto tech1 = db_->findTech("childLef1");
  auto techLayer1 = tech1->findLayer("M2");

  auto lib2 = db_->findLib("example1-2");
  auto tech2 = db_->findTech("childLef2");
  auto techLayer2 = tech1->findLayer("M2");

  auto childBlock1 = odb::dbBlock::create(topHeirBlock, "childBlock1", tech1);
  auto childBlock2 = odb::dbBlock::create(topHeirBlock, "childBlock2", tech2);
  childBlock1->setDieArea(dieBox);
  childBlock2->setDieArea(dieBox);

  auto dieInst1 = odb::dbInst::create(topHeirBlock, childBlock1, "childDie1");
  auto dieInst2 = odb::dbInst::create(topHeirBlock, childBlock2, "childDie2");
  dieInst1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  dieInst2->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  odb::dbMaster* master1 = nullptr;
  if (lib1) {
    master1 = lib1->findMaster("INV_X1");
  } else {
    logger_->error(utl::MDM, 5, "Cannot find lib1");
  }

  odb::dbMaster* master2 = nullptr;
  if (lib2) {
    master2 = lib2->findMaster("AND2_X1");
  } else {
    logger_->error(utl::MDM, 6, "Cannot find lib2");
  }

  odb::dbMaster* masterFF1 = nullptr;
  if (lib1) {
    masterFF1 = lib1->findMaster("DFF_X1");
  }
  odb::dbMaster* masterFF2 = nullptr;
  if (lib2) {
    masterFF2 = lib2->findMaster("DFF_X1");
  }

  auto inst1 = odb::dbInst::create(childBlock1, master1, "inst1");
  auto inst2 = odb::dbInst::create(childBlock2, master2, "inst2");
  auto flipFlop1 = odb::dbInst::create(childBlock1, masterFF1, "flipFlop1");
  auto flipFlop2 = odb::dbInst::create(childBlock2, masterFF2, "flipFlop2");

  // set the position of instances
  inst1->setLocation(ur.x() * 4 / 10, ur.y() / 2);
  inst2->setLocation(ur.x() * 6 / 10, ur.y() / 2);
  flipFlop1->setLocation(ur.x() * 2 / 10, ur.y() / 2);
  flipFlop2->setLocation(ur.x() * 8 / 10, ur.y() / 2);

  inst1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  inst2->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  flipFlop1->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  flipFlop2->setPlacementStatus(odb::dbPlacementStatus::PLACED);

  // make a net
  auto net1 = odb::dbNet::create(childBlock1, "net1");
  auto net2 = odb::dbNet::create(childBlock2, "net2");
  auto netTopHeir = odb::dbNet::create(topHeirBlock, "netTopHeir");

  // mark these nets are intersected net
  odb::dbBoolProperty::create(netTopHeir, "intersected", true);
  odb::dbBoolProperty::create(net1, "intersected", true);
  odb::dbBoolProperty::create(net2, "intersected", true);

  // connect the net to the instance
  inst1->findITerm("ZN")->connect(net1);
  inst2->findITerm("A1")->connect(net2);

  // connect the intersected net between two dies
  auto bTerm1 = odb::dbBTerm::create(net1, "bTerm1");
  auto bTerm2 = odb::dbBTerm::create(net2, "bTerm2");

  bTerm1->getITerm()->connect(netTopHeir);
  bTerm2->getITerm()->connect(netTopHeir);

  // make the interconnection pins and position in child die
  auto bPin1 = odb::dbBPin::create(bTerm1);
  auto bPin2 = odb::dbBPin::create(bTerm2);
  auto child1PinX1 = ur.x() * 5 / 10;
  auto child1PinX2 = ur.x() * 5 / 10 + 5 * dbUnit;
  auto child1PinY1 = ur.y() * 5 / 10;
  auto child1PinY2 = ur.y() * 5 / 10 + 5 * dbUnit;

  auto child2PinX1 = ur.x() * 5 / 10;
  auto child2PinX2 = ur.x() * 5 / 10 + 5 * dbUnit;
  auto child2PinY1 = ur.y() * 5 / 10;
  auto child2PinY2 = ur.y() * 5 / 10 + 5 * dbUnit;

  odb::dbBox::create(
      bPin1, techLayer1, child1PinX1, child1PinY1, child1PinX2, child1PinY2);
  odb::dbBox::create(
      bPin2, techLayer2, child2PinX1, child2PinY1, child2PinX2, child2PinY2);

  auto topHierMPin1 = odb::dbMPin::create(bTerm1->getITerm()->getMTerm());
  auto topHierMPin2 = odb::dbMPin::create(bTerm2->getITerm()->getMTerm());
  odb::dbBox::create(topHierMPin1,
                     techLayer1,
                     child1PinX1,
                     child1PinY1,
                     child1PinX2,
                     child1PinY2);

  odb::dbBox::create(topHierMPin2,
                     techLayer2,
                     child2PinX1,
                     child2PinY1,
                     child2PinX2,
                     child2PinY2);

  ///////////// I/O construction ////////////////
  //////////////input port/////////////
  int inputPinX1 = 0;
  int inputPinX2 = 5 * dbUnit;
  int inputPinY1 = ur.y() / 2;
  int inputPinY2 = ur.y() / 2 + 5 * dbUnit;
  int outputPinX1 = ur.x() - 5 * dbUnit;
  int outputPinX2 = ur.x();
  int outputPinY1 = ur.y() / 2 + 5 * dbUnit;
  int outputPinY2 = ur.y() / 2;

  auto inputNetTopHier = odb::dbNet::create(topHeirBlock, "inputNetTopHier");
  auto inputBTermTopHier
      = odb::dbBTerm::create(inputNetTopHier, "inputTopHier");
  auto inputBPinTopHier = odb::dbBPin::create(inputBTermTopHier);

  odb::dbBox::create(inputBPinTopHier,
                     techLayerTopHier,
                     inputPinX1,
                     inputPinY1,
                     inputPinX2,
                     inputPinY2);

  auto inputNetChild = odb::dbNet::create(childBlock1, "inputNetChildBlock");
  auto inputBTermChild = odb::dbBTerm::create(inputNetChild, "inputBTermChild");
  auto inputBPinChild = odb::dbBPin::create(inputBTermChild);
  odb::dbBox::create(inputBPinChild,
                     techLayer1,
                     inputPinX1,
                     inputPinY1,
                     inputPinX2,
                     inputPinY2);

  auto inputMPinTopHier
      = odb::dbMPin::create(inputBTermChild->getITerm()->getMTerm());
  odb::dbBox::create(inputMPinTopHier,
                     techLayerTopHier,
                     inputPinX1,
                     inputPinY1,
                     inputPinX2,
                     inputPinY2);

  inputBTermChild->getITerm()->connect(inputNetTopHier);
  ////////////////output port/////////////
  auto outputNetChild = odb::dbNet::create(childBlock2, "outputNetChildBlock");
  auto outputBTermChild
      = odb::dbBTerm::create(outputNetChild, "outputBTermChild");
  auto outputBPinChild = odb::dbBPin::create(outputBTermChild);
  odb::dbBox::create(outputBPinChild,
                     techLayer2,
                     outputPinX1,
                     outputPinY1,
                     outputPinX2,
                     outputPinY2);

  auto outputMPinTopHier
      = odb::dbMPin::create(outputBTermChild->getITerm()->getMTerm());
  odb::dbBox::create(outputMPinTopHier,
                     techLayerTopHier,
                     outputPinX1,
                     outputPinY1,
                     outputPinX2,
                     outputPinY2);

  auto outputNetTopHier = odb::dbNet::create(topHeirBlock, "outputNetTopHier");
  auto outputBTermTopHier
      = odb::dbBTerm::create(outputNetTopHier, "outputTopHier");
  auto outputPinTopHier = odb::dbBPin::create(outputBTermTopHier);
  odb::dbBox::create(outputPinTopHier,
                     techLayerTopHier,
                     outputPinX1,
                     outputPinY1,
                     outputPinX2,
                     outputPinY2);

  outputBTermChild->getITerm()->connect(outputNetTopHier);

  inputBTermTopHier->setIoType(odb::dbIoType::INPUT);
  outputBTermTopHier->setIoType(odb::dbIoType::OUTPUT);

  //////////////////////////clock construction////////////////////////////
  int clkPinX1 = ur.x() / 2;
  int clkPinX2 = ur.x() / 2 + 5 * dbUnit;
  int clkPinY1 = 0;
  int clkPinY2 = 5 * dbUnit;

  // construct clk input pin & clk net
  auto clkNetTopHier = odb::dbNet::create(topHeirBlock, "clkNetTopHier");
  auto clkBTermTopHier = odb::dbBTerm::create(clkNetTopHier, "clkPort");
  auto clkBPinTopHier = odb::dbBPin::create(clkBTermTopHier);
  odb::dbBox::create(
      clkBPinTopHier, techLayerTopHier, clkPinX1, clkPinY1, clkPinX2, clkPinY2);

  // construct clk input pin & clk net in the child block
  auto clkNetChild1 = odb::dbNet::create(childBlock1, "clkNetChildBlock1");
  auto clkNetChild2 = odb::dbNet::create(childBlock2, "clkNetChildBlock2");
  auto clkBTermChild1 = odb::dbBTerm::create(clkNetChild1, "clkBTermChild1");
  auto clkBTermChild2 = odb::dbBTerm::create(clkNetChild2, "clkBTermChild2");
  auto clkBPinChild1 = odb::dbBPin::create(clkBTermChild1);
  odb::dbBox::create(
      clkBPinChild1, techLayer1, clkPinX1, clkPinY1, clkPinX2, clkPinY2);
  auto clkBPinChild2 = odb::dbBPin::create(clkBTermChild2);
  odb::dbBox::create(
      clkBPinChild2, techLayer2, clkPinX1, clkPinY1, clkPinX2, clkPinY2);

  // interconnection between top and child block
  auto clkMPin1TopHier
      = odb::dbMPin::create(clkBTermChild1->getITerm()->getMTerm());
  odb::dbBox::create(clkMPin1TopHier,
                     techLayerTopHier,
                     clkPinX1,
                     clkPinY1,
                     clkPinX2,
                     clkPinY2);

  auto clkMPin2TopHier
      = odb::dbMPin::create(clkBTermChild2->getITerm()->getMTerm());
  odb::dbBox::create(clkMPin2TopHier,
                     techLayerTopHier,
                     clkPinX1,
                     clkPinY1,
                     clkPinX2,
                     clkPinY2);

  // connect clk pin and interconnection
  clkBTermChild1->getITerm()->connect(clkNetTopHier);
  clkBTermChild2->getITerm()->connect(clkNetTopHier);

  // connect clk pin in child block and flip-flops
  flipFlop1->findITerm("CK")->connect(clkNetChild1);
  flipFlop2->findITerm("CK")->connect(clkNetChild2);

  // connect input pin in child block and flip-flop input
  flipFlop1->findITerm("D")->connect(inputNetChild);

  // input signal,
  // connect the output of flip-flop and input of and gate
  auto fToInv1 = odb::dbNet::create(childBlock1, "fToInv1");
  flipFlop1->findITerm("Q")->connect(fToInv1);
  inst1->findITerm("A")->connect(fToInv1);

  // output signal,
  // connect the output of and gate and input of flip-flop
  auto andToF2 = odb::dbNet::create(childBlock2, "andToF2");
  inst2->findITerm("ZN")->connect(andToF2);
  flipFlop2->findITerm("D")->connect(andToF2);

  // coonect output pin in the child block and flip-flop output
  flipFlop2->findITerm("Q")->connect(outputNetChild);

  clkBTermTopHier->setIoType(odb::dbIoType::INPUT);

  // row construction
  rowConstruction(dieWidth,
                  dieHeight,
                  topHeirBlock,
                  childBlock1,
                  childBlock2,
                  lib1,
                  lib2,
                  inst1,
                  inst2);
}
void MultiDieManager::rowConstruction(int dieWidth,
                                      int dieHeight,
                                      odb::dbBlock* topHeirBlock,
                                      odb::dbBlock* childBlock1,
                                      odb::dbBlock* childBlock2,
                                      odb::dbLib* lib1,
                                      odb::dbLib* lib2,
                                      const odb::dbInst* inst1,
                                      const odb::dbInst* inst2) const
{  // row construction //
  auto site1 = lib1->findSite("site1");
  auto site2 = lib2->findSite("site1");
  auto site_width1 = inst1->getMaster()->getWidth();
  auto site_height1 = inst1->getMaster()->getHeight();
  auto site_width2 = inst2->getMaster()->getWidth();
  auto site_height2 = inst2->getMaster()->getHeight();

  site1->setWidth(site_width1);
  site2->setWidth(site_width2);
  site1->setHeight(site_height1);
  site2->setHeight(site_height2);

  auto numOfRow1 = dieHeight / site_height1;
  auto numOfSite1 = dieWidth / site_width1;
  auto numOfRow2 = dieHeight / site_height2;
  auto numOfSite2 = dieWidth / site_width2;

  for (int i = 0; i < numOfRow1; ++i) {
    odb::dbRow::create(topHeirBlock,
                       ("row" + to_string(i)).c_str(),
                       site1,
                       0,
                       i * site_height1,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite1,
                       site_width1);
  }
  for (int i = 0; i < numOfRow1; ++i) {
    odb::dbRow::create(childBlock1,
                       ("row" + to_string(i)).c_str(),
                       site1,
                       0,
                       i * site_height1,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite1,
                       site_width1);
  }

  for (int i = 0; i < numOfRow2; ++i) {
    odb::dbRow::create(childBlock2,
                       ("row" + to_string(i)).c_str(),
                       site1,
                       0,
                       i * site_height2,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSite2,
                       site_width2);
  }
  // row construction end //
}

void MultiDieManager::test()
{
  timingTest1();
}

}  // namespace mdm