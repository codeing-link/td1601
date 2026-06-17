.PHONY: clean All Project_Title Project_PreBuild chip_td1601 console csi minilibc mm td1601_evb Project_Build Project_PostBuild

All: Project_Title Project_PreBuild chip_td1601 console csi minilibc mm td1601_evb Project_Build Project_PostBuild

Project_Title:
	@echo "----------Building project:[ gpio_toggle - BuildSet ]----------"

Project_PreBuild:
	@echo Executing Pre Build commands ...
	@export BOARD_PATH="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/boards/td1601_evb/v1.0" CDKPath="C:/C-Sky/CDK" CDK_VERSION="V2.22.0" CHIP_PATH="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/components/chips/chip_td1601/v1.0" ProjectName="gpio_toggle" ProjectPath="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/projects/drivers/gpio/gpio_toggle/" SOLUTION_PATH="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/projects/drivers/gpio/gpio_toggle/" chip_td1601="V1.0.0" console="V1.0.0" csi="V1.0.0" minilibc="V1.0.0" mm="V1.0.0" td1601_evb="V1.0.0" && "E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/projects/drivers/gpio/gpio_toggle/utilities/pre_build.sh"
	@echo Done

chip_td1601:
	@make -r -f Obj/BuildSet/Packages/chip_td1601/V1.0.0/Makefile -j 16 -C  ./ 

console:
	@make -r -f Obj/BuildSet/Packages/console/V1.0.0/Makefile -j 16 -C  ./ 

csi:
	@make -r -f Obj/BuildSet/Packages/csi/V1.0.0/Makefile -j 16 -C  ./ 

minilibc:
	@make -r -f Obj/BuildSet/Packages/minilibc/V1.0.0/Makefile -j 16 -C  ./ 

mm:
	@make -r -f Obj/BuildSet/Packages/mm/V1.0.0/Makefile -j 16 -C  ./ 

td1601_evb:
	@make -r -f Obj/BuildSet/Packages/td1601_evb/V1.0.0/Makefile -j 16 -C  ./ 


Project_Build:
	@make -r -f gpio_toggle.mk -j 16 -C  ./ 

Project_PostBuild:
	@echo Executing Post Build commands ...
	@export BOARD_PATH="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/boards/td1601_evb/v1.0" CDKPath="C:/C-Sky/CDK" CDK_VERSION="V2.22.0" CHIP_PATH="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/components/chips/chip_td1601/v1.0" ProjectName="gpio_toggle" ProjectPath="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/projects/drivers/gpio/gpio_toggle/" SOLUTION_PATH="E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/projects/drivers/gpio/gpio_toggle/" chip_td1601="V1.0.0" console="V1.0.0" csi="V1.0.0" minilibc="V1.0.0" mm="V1.0.0" td1601_evb="V1.0.0" && "E:/Nine-StarMCU/CDK/td1601_evb_v0.1.4_cdk/projects/drivers/gpio/gpio_toggle/utilities/aft_build.sh"
	@echo Done


clean:
	@echo "----------Cleaning project:[ gpio_toggle - BuildSet ]----------"

