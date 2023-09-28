cd ../../src/mdm/test
read_lef ispd18/ispd18_test1/ispd18_test1.input.lef
read_def ispd18/ispd18_test1/ispd18_test1.input.def
set_3D_IC -die_number 2
mdm::make_shrunkLef

write_lef shrunkLefs/ispd18_test1.lef