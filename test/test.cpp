#include <iostream>

#include "db.h"
#include "defin.h"
#include "lefin.h"
#include "lefout.h"

using namespace odb;  // NOLINT(*-build-using-namespace)
using namespace std;  // NOLINT(*-build-using-namespace)
int main()
{
  dbDatabase* db_database = dbDatabase::create();
  dbTech* tech1 = dbTech::create(db_database, "top_tech");
  dbTech* tech2 = dbTech::create(db_database, "bottom_tech");
  auto* layer1
      = dbTechLayer::create(tech1, "top_layer", dbTechLayerType::MASTERSLICE);
  auto* layer2
      = dbTechLayer::create(tech2, "bottom_layer", dbTechLayerType::ROUTING);
  dbLib* lib1 = dbLib::create(db_database, "top_lib", tech1);
  dbLib* lib2 = dbLib::create(db_database, "bottom_lib", tech2);
  dbChip* chip = dbChip::create(db_database);
  dbBlock* block1 = dbBlock::create(chip, "top", tech1);
  dbBlock* block2 = dbBlock::create(block1, "bottom", tech2);
  // die size setting
  Rect die_rect(0, 0, 1000, 1000);
  block1->setDieArea(die_rect);
  block2->setDieArea(die_rect);

  vector<dbInst*> inst_collector_top;
  vector<dbInst*> inst_collector_bottom;

  // Example Library cell construction - top
  int top_lib_cell_num = 3;
  for (int i = 0; i < top_lib_cell_num; ++i) {
    int width = 100 + i * 10;
    int height = 200;
    dbMaster* master
        = dbMaster::create(lib1, ("top_cell_" + to_string(i)).c_str());
    master->setWidth(width);
    master->setHeight(height);
    master->setType(dbMasterType::CORE);

    int pin_num = 2 + i;
    for (int j = 0; j < pin_num; ++j) {
      dbSigType sig_type = dbSigType::SIGNAL;
      dbIoType io_type;
      if (j < pin_num - 1)
        io_type = dbIoType::INPUT;
      else
        io_type = dbIoType::OUTPUT;
      string pin_name = "top_pin_" + to_string(j);
      dbMTerm* master_terminal
          = dbMTerm::create(master, pin_name.c_str(), io_type, sig_type);
      dbMPin* master_pin = dbMPin::create(master_terminal);
      dbBox::create(master_pin, layer1, j * 2, j * 2, j * 2 + 1, j * 2 + 1);
    }
    master->setFrozen();
  }

  // Example Instance cells for top
  int instance_num = 10;
  // pick random number between 0 and top_lib_cell_num - 1, with fixed seed.
  srand(0);  // NOLINT(*-msc51-cpp)
  for (int i = 0; i < instance_num; ++i) {
    int lib_cell_id = rand() % top_lib_cell_num;  // NOLINT(*-msc50-cpp)
    int x, y;
    // set the x and y coordinate of the instance in the die area randomly.
    x = rand() % 1000;  // NOLINT(*-msc50-cpp)
    y = rand() % 1000;  // NOLINT(*-msc50-cpp)
    string inst_name = "top_inst_" + to_string(i);
    dbMaster* master = db_database->findMaster(
        ("top_cell_" + to_string(lib_cell_id)).c_str());
    auto inst = dbInst::create(block1, master, inst_name.c_str());
    inst->setLocation(x, y);
    inst_collector_top.push_back(inst);
  }

  // Example Library cell construction - bottom
  int bottom_lib_cell_num = 4;
  for (int i = 0; i < bottom_lib_cell_num; ++i) {
    int width = 200 + i * 20;
    int height = 300;
    dbMaster* master
        = dbMaster::create(lib2, ("bottom_cell_" + to_string(i)).c_str());
    master->setWidth(width);
    master->setHeight(height);
    master->setType(dbMasterType::CORE);

    int pin_num = 3 + i;
    for (int j = 0; j < pin_num; ++j) {
      dbSigType sig_type = dbSigType::SIGNAL;
      dbIoType io_type;
      if (j < pin_num - 1)
        io_type = dbIoType::INPUT;
      else
        io_type = dbIoType::OUTPUT;
      string pin_name = "bottom_pin_" + to_string(j);
      dbMTerm* master_terminal
          = dbMTerm::create(master, pin_name.c_str(), io_type, sig_type);
      dbMPin* master_pin = dbMPin::create(master_terminal);
      dbBox::create(master_pin, layer2, j * 2, j * 2, j * 2 + 1, j * 2 + 1);
    }
    master->setFrozen();
  }

  // Example Instance cells for bottom
  for (int i = 0; i < instance_num; ++i) {
    int lib_cell_id = rand() % top_lib_cell_num;  // NOLINT(*-msc50-cpp)
    int x, y;
    // set the x and y coordinate of the instance in the die area randomly.
    x = rand() % 1000;  // NOLINT(*-msc50-cpp)
    y = rand() % 1000;  // NOLINT(*-msc50-cpp)
    string inst_name = "bottom_inst_" + to_string(i);
    dbMaster* master = db_database->findMaster(
        ("bottom_cell_" + to_string(lib_cell_id)).c_str());
    auto inst = dbInst::create(block2, master, inst_name.c_str());
    inst->setLocation(x, y);
    inst_collector_bottom.push_back(inst);
  }

  int inst_id1 = 5;
  int inst_id2 = 4;

  // intersected net test
  dbNet* net = dbNet::create(block1, ("net_" + to_string(inst_id1)).c_str());
  dbInst* inst1_top = inst_collector_top.at(inst_id1);
  dbInst* inst2_top = inst_collector_top.at(inst_id2);
  dbInst* inst1_bottom = inst_collector_bottom.at(inst_id1);
  dbITerm* i_term_top1 = inst1_top->findITerm("top_pin_0");
  dbITerm* i_term_top2 = inst2_top->findITerm("top_pin_0");
  dbITerm* i_term_bottom = inst1_bottom->findITerm("bottom_pin_0");
  i_term_top1->connect(net);
  i_term_top2->connect(net);
  i_term_bottom->connect(net);

  cout << "inst1_top: " << inst1_top->getName() << '\n';
  cout << "inst2_top: " << inst2_top->getName() << '\n';
  cout << "inst1_bottom: " << inst1_bottom->getName() << '\n' << '\n';

  cout << "connected instance number: " << net->getITermCount() << '\n';
  for (auto i_term : net->getITerms()) {
    cout << i_term->getInst()->getName() << '\n';
  }

  /**
   * Console result:
   * \code
      inst1_top: top_inst_5
      inst2_top: top_inst_4
      inst1_bottom: bottom_inst_5

      connected instance number: 1
      top_inst_6
   * */
}
