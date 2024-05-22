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
#include "odb/defout.h"
#include "odb/lefout.h"
namespace mdm {
using namespace std;
void MultiDieManager::multiDieDPL()
{
  auto odp = new dpl::Opendp();
  odp->init(db_, logger_);
  odp->detailedPlacement(0, 0, "", false, db_->getChip()->getBlock());
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    odp->detailedPlacement(0, 0, "", false, block);
  }
}

void MultiDieManager::multiDieDPO()
{
  auto dpo = new dpo::Optdp;
  auto odp = new dpl::Opendp();
  odp->init(db_, logger_);
  dpo->init(db_, logger_, odp);
  dpo->improvePlacement(1, 0, 0, db_->getChip()->getBlock(), false);
  for (auto block : db_->getChip()->getBlock()->getChildren()) {
    dpo->improvePlacement(1, 0, 0, block, false);
  }
}

void MultiDieManager::interconnectionLegalize(uint gridSize)
{
  // grid size means the size of the grid for interconnection

  auto odp = new dpl::Opendp();
  // make new fake db for interconnection legalization
  auto db = odb::dbDatabase::create();
  auto chip = odb::dbChip::create(db);
  auto tech = odb::dbTech::create(db, "fakeTech");
  auto lib = odb::dbLib::create(db, "fakeLib", tech);
  auto block = odb::dbBlock::create(chip, "fakeBlock", tech);

  block->setDieArea(db_->getChip()->getBlock()->getDieArea());

  // make fake master for interconnection
  odb::dbSite* site = odb::dbSite::create(lib, "Site");
  site->setWidth(gridSize);
  site->setHeight(gridSize);
  auto master = odb::dbMaster::create(lib, "interconnectionShape");
  master->setHeight(gridSize);
  master->setWidth(gridSize);
  master->setType(odb::dbMasterType::CORE);
  master->setSite(site);
  master->setFrozen();

  for (const auto& interconnectionInfo : temporaryInterconnectCoordinates_) {
    auto interconnectionNet = interconnectionInfo.first;
    auto interconnectionCoordinate = interconnectionInfo.second;
    auto inst = odb::dbInst::create(
        block, master, ("instance_" + interconnectionNet->getName()).c_str());
    inst->setLocation(interconnectionCoordinate.getX(),
                      interconnectionCoordinate.getY());
    inst->setPlacementStatus("PLACED");
    // logger_->info(utl::MDM, 32, "interconnectionCoordinate: {}, {}", inst->getLocation().getX(), inst->getLocation().getY());
  }

  // make rows for interconnections
  int numOfSites = block->getDieArea().dx() / gridSize;
  int numOfRows = block->getDieArea().dy() / gridSize;

  for (int i = 0; i < numOfRows; ++i) {
    odb::dbRow::create(block,
                       ("row" + to_string(i)).c_str(),
                       site,
                       0,
                       i * gridSize,
                       odb::dbOrientType::MX,
                       odb::dbRowDir::HORIZONTAL,
                       numOfSites,
                       numOfSites);
  }

  // write def file for debugging
  odb::defout def_writer(logger_);
  def_writer.setVersion(odb::defout::Version::DEF_5_8);
  def_writer.writeBlock(block, "interconnectionLegalizationBefore.def");

  // write lef file for debugging
  std::ofstream os;
  os.open("interconnectionLegalizationBefore.lef");
  odb::lefout lef_writer(logger_, os);
  lef_writer.writeLib(lib);

  // do interconnection legalization
  odp->init(db, logger_);
  odp->detailedPlacement(0, 0, "", false, block);

  // apply the placement to member variable
  for (auto inst : block->getInsts()) {
    auto instName = inst->getConstName();
    temporaryInterconnectCoordinateMap_[db_->getChip()->getBlock()->findNet(
        instName)]
        = odb::Point(inst->getLocation().getX(), inst->getLocation().getY());
  }

  // write def file for debugging
  def_writer.setVersion(odb::defout::Version::DEF_5_8);
  def_writer.writeBlock(block, "interconnectionLegalizationAfter.def");  
}

void MultiDieManager::runSemiLegalizer(char* targetDie)
{
  SemiLegalizer semiLegalizer{};
  semiLegalizer.setDb(db_);
  semiLegalizer.run(true, targetDie);
}

}  // namespace mdm