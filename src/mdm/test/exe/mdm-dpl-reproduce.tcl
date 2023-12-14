# parse case3 iPL result
# scaled up to 30 times

cd ../../src/mdm/test
mdm::setICCADScale 100
ICCADParse -test_case 2022-test3
mdm::setPartitionFile exe/case3-output.txt.par
set_3D_IC -die_number 2
gui::design_created

mdm::parseICCADOutput exe/case3_gp.txt

mdm::get3DHPWL
mdm::multiDieDetailPlace
