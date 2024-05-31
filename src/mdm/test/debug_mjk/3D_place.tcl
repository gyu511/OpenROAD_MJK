set design aes_cipher_top
set partition GNN
set partition TritonPart

##### READ LEF for top hierarchy die #####
read_lef -tech_name top_hier_tech /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/asap7_6T_tech.lef


##### READ DEF #####
read_def /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/floorplan_3D_clk.def

read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/ADDER_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/AO_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/CKINVDC_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/INVBUF_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/OA_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/PHY_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/SEQ_CUSTOM_4_372_0_70_6T.lib
read_liberty /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/6T/SIMPLE_CUSTOM_4_372_0_70_6T.lib


##### IO Place #####
place_pins -hor_layers M6 -ver_layers M5

##### READ LEFs for each die #####
read_lef -tech_name top -tech -lib /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/asap7_6T_tech_die1.lef
read_lef -tech_name bottom -tech -lib /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/asap7_6T_tech_die2.lef

##### READ PARTITION INFO #####
#파티션 파일 읽어옴(cell name, die id) 안하면 자동으로 반
mdm::setPartitionFile /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/aes_cipher_top_last.par

##### SET 3D #####
set_3D_IC -die_number 2
gui::design_created

##### READ INTERCONNECT CRITICAL PATH INFO #####
mdm::readCriticalCell /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk/aes_cipher_top_clritical_cell_last.par
mdm::setNetWeight 1e-5

##### Place #####
#중간과정 보기위해, 안볼거면 debug 필요없음
# gpl::global_placement_debug

# io pin이 없으면 initial placement가 의미가 없어서
global_placement -skip_initial_place

cd /home/minjae/CLionProjects/OpenROAD_MJK/src/mdm/test/debug_mjk
mdm::exportInstCoordinates coordi.txt

# mdm::setInterconnectCoordinates 100

# mdm::multiDieDPL

# mdm::get3DHPWL
