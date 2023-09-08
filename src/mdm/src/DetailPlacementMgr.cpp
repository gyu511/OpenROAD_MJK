#include "mdm/MultiDieManager.hh"
namespace mdm {
using namespace std;
void MultiDieManager::twoDieDetailPlacement()
{
  vector<odb::dbDatabase*> target_db_set;
  for (int i = 0; i < 2; ++i) {
    if (i == 0) {
      constructionDbForOneDie(TOP);
      detailPlacement();
      applyDetailPlacementResult();
      target_db_ = nullptr;
    } else if (i == 1) {
      constructionDbForOneDie(BOTTOM);
      detailPlacement();
      applyDetailPlacementResult();
      target_db_set.push_back(target_db_);
      target_db_ = nullptr;
    }
  }

    for(auto target_db: target_db_set){
      odb::dbDatabase::destroy(target_db);
    }
}

void MultiDieManager::constructionDbForOneDie(WHICH_DIE which_die)
{
  int die_id = 0;
  if (which_die == TOP) {
    die_id = 0;
  } else if (which_die == BOTTOM) {
    die_id = 1;
  }

  vector<odb::dbInst*> inst_set_exclusive;
  for (auto chip : db_->getChips()) {
    auto block = chip->getBlock();
    for (auto inst : block->getInsts()) {
      auto partition_info = odb::dbIntProperty::find(inst, "which_die");
      if (partition_info->getValue() != die_id) {
        inst_set_exclusive.push_back(inst);
      }
    }
  }
  target_db_ = odb::dbDatabase::duplicate(db_);

  for (auto exclusive_inst : inst_set_exclusive) {
    auto inst = target_db_->getChip()->getBlock()->findInst(
        exclusive_inst->getName().c_str());
    odb::dbInst::destroy(inst);
  }
}

void MultiDieManager::detailPlacement()
{
  auto* odp = new dpl::Opendp();
  odp->init(target_db_, logger_);
  odp->detailedPlacement(0, 0);
}

void MultiDieManager::applyDetailPlacementResult()
{
  for (auto inst : target_db_->getChip()->getBlock()->getInsts()) {
    auto original_inst
        = db_->getChip()->getBlock()->findInst(inst->getName().c_str());
    auto original_location = inst->getLocation();
    original_inst->setLocation(original_location.getX(),
                               original_location.getY());
  }
}
}  // namespace mdm