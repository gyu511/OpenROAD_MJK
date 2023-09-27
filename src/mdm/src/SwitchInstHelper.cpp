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
namespace mdm {
using namespace std;
int SwitchInstanceHelper::findAssignedDieId(odb::dbInst* inst)
{
  auto partitionInfo = odb::dbIntProperty::find(inst, "partition_id");
  auto dieId = partitionInfo->getValue();
  return dieId;
}

std::pair<odb::dbBlock*, odb::dbLib*> SwitchInstanceHelper::findTargetDieAndLib(
    MultiDieManager* manager,
    int dieID)
{
  odb::dbBlock* targetBlock = nullptr;
  odb::dbLib* targetLib = nullptr;

  int dieIdx = 0;
  for (auto child_die : manager->db_->getChip()->getBlock()->getChildren()) {
    if (dieIdx == dieID) {
      targetBlock = child_die;
      break;
    }
    dieIdx++;
  }
  int libIdx = 0;
  for (auto lib : manager->db_->getLibs()) {
    if (libIdx == dieID) {
      targetLib = lib;
      break;
    }
    libIdx++;
  }

  return {targetBlock, targetLib};
}
void SwitchInstanceHelper::inheritPlacementInfo(odb::dbInst* originalInst,
                                                odb::dbInst* newInst)
{
  auto placementStatus = originalInst->getPlacementStatus();
  newInst->setPlacementStatus(placementStatus);
  if (placementStatus != odb::dbPlacementStatus::NONE) {
    odb::Point location = originalInst->getLocation();
    newInst->setLocation(location.x(), location.y());
  }
}
void SwitchInstanceHelper::inheritNetInfo(odb::dbInst* originalInst,
                                          odb::dbInst* newInst)
{
  vector<pair<string, odb::dbNet*>> netInfoContainer;
  auto targetBlock = newInst->getBlock();
  for (auto iTerm : originalInst->getITerms()) {
    if (!iTerm->getNet()) {
      continue;
    }
    auto pinName = iTerm->getMTerm()->getName();
    auto connectedNet = iTerm->getNet();
    netInfoContainer.emplace_back(pinName, connectedNet);
  }
  for (const auto& netInfo : netInfoContainer) {
    auto iTerm = newInst->findITerm(netInfo.first.c_str());
    auto originalNet = netInfo.second;
    assert(iTerm);

    // find the net in the target block
    // if it doesn't exist, then make the new one
    auto newNet = targetBlock->findNet(originalNet->getName().c_str());
    if (!newNet) {
      newNet = odb::dbNet::create(targetBlock, originalNet->getName().c_str());
      assert(newNet);
    }
    iTerm->connect(newNet);
  }
}
void SwitchInstanceHelper::inheritProperties(odb::dbInst* originalInst,
                                             odb::dbInst* newInst)
{
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
  for (auto property : odb::dbProperty::getProperties(originalInst)) {
    if (property->getType() == odb::dbProperty::INT_PROP) {
      property_storage.int_properties.push_back(
          odb::dbIntProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_int_properties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::STRING_PROP) {
      property_storage.string_properties.push_back(
          odb::dbStringProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_string_properties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::DOUBLE_PROP) {
      property_storage.double_properties.push_back(
          odb::dbDoubleProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_double_properties.push_back(property->getName());
    } else if (property->getType() == odb::dbProperty::BOOL_PROP) {
      property_storage.bool_properties.push_back(
          odb::dbBoolProperty::find(originalInst, property->getName().c_str())
              ->getValue());
      property_storage.name_of_bool_properties.push_back(property->getName());
    }
  }
  for (int i = 0; i < property_storage.int_properties.size(); ++i) {
    odb::dbIntProperty::create(
        newInst,
        property_storage.name_of_int_properties.at(i).c_str(),
        property_storage.int_properties.at(i));
  }
  for (int i = 0; i < property_storage.string_properties.size(); ++i) {
    odb::dbStringProperty::create(
        newInst,
        property_storage.name_of_string_properties.at(i).c_str(),
        property_storage.string_properties.at(i).c_str());
  }
  for (int i = 0; i < property_storage.double_properties.size(); ++i) {
    odb::dbDoubleProperty::create(
        newInst,
        property_storage.name_of_double_properties.at(i).c_str(),
        property_storage.double_properties.at(i));
  }
  for (int i = 0; i < property_storage.bool_properties.size(); ++i) {
    odb::dbBoolProperty::create(
        newInst,
        property_storage.name_of_bool_properties.at(i).c_str(),
        property_storage.bool_properties.at(i));
  }
}
void SwitchInstanceHelper::inheritGroupInfo(odb::dbInst* originalInst,
                                            odb::dbInst* newInst)
{
  odb::dbGroup* group = originalInst->getGroup();
  if (!group) {
    return;
  }
  group->addInst(newInst);
}
}  // namespace mdm