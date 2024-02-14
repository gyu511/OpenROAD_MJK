cd ../../src/mdm/test
mdm::ICCADParse 2022-test4

mdm::parseICCADOutput ICCAD/2022/reproduce/case4-gp.txt
mdm::setPartitionFile ICCAD/2022/reproduce/case4-gp.txt.par

set_3D_IC -die_number 2
gui::design_created

mdm::get3DHPWL
mdm::runSemiLegalizer
mdm::get3DHPWL
