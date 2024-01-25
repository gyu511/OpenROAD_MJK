cd ../../src/mdm/test
mdm::ICCADParse 2022-test2 true

mdm::setPartitionFile ICCAD/2022/partitionInfo/2022-case2-ipl.par

set_3D_IC -die_number 2
gui::design_created

global_placement -skip_initial_place -density 1
mdm::multiDieDetailPlace
