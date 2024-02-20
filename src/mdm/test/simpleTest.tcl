cd ../../src/mdm/test

read_lef -tech_name top_hier_tech multiBlock/ispd18_test1.input.lef
read_def multiBlock/ispd18_test1.input.def

read_lef -tech_name top -tech -lib multiBlock/ispd18_test1.input1.lef
read_lef -tech_name bottom -tech -lib multiBlock/ispd18_test1.input2.lef

set_3D_IC -die_number 2

gui::design_created

mdm::setInterconnectCoordinates


# get the guide files
set childBlock1 [mdm::getChildBlock 0]
grt::set_block $childBlock1
grt::set_multi_block_mode
grt::read_guides ispd18/ispd18_test1/ispd18_test1.input.guide

set childBlock2 [mdm::getChildBlock 0]
grt::set_block $childBlock2
grt::set_multi_block_mode
grt::read_guides ispd18/ispd18_test1/ispd18_test1.input.guide