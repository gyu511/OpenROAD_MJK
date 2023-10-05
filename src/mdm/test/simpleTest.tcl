cd ../../src/mdm/test

read_lef -tech_name top -tech -lib ispd18/ispd18_test1/ispd18_test1.input1.lef
read_lef -tech_name bottom -tech -lib ispd18/ispd18_test1/ispd18_test1.input2.lef

read_def -tech top ispd18/ispd18_test1/ispd18_test1.input.def

set_3D_IC -die_number 2

gui::design_created