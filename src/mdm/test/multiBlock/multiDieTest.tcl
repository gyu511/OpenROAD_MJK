# cd ../../src/mdm/test/multiBlock

set db [ord::get_db]
set chip [odb::dbChip_create $db]
set tech [odb::dbTech_create $db tech]
set top_block [odb::dbBlock_create $chip top_heir]

read_lef -tech_name top -tech -lib ispd18_test1.input1.lef
read_lef -tech_name bottom -tech -lib ispd18_test1.input2.lef

read_def -child -tech top ispd18_test1.input.def

set_3D_IC -die_number 2