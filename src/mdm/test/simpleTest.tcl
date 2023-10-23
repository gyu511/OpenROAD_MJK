cd ../../src/mdm/test

read_lef -tech_name top_hier_tech multiBlock/ispd18_test1.input.lef
read_def multiBlock/ispd18_test1.input.def

read_lef -tech_name top -tech -lib multiBlock/ispd18_test1.input1.lef
read_lef -tech_name bottom -tech -lib multiBlock/ispd18_test1.input2.lef

set_3D_IC -die_number 2

gui::design_created

#gpl::global_placement_debug
#global_placement -skip_initial_place