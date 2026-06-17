.PHONY: clean All Project_Title Project_PreBuild chip_td1601 console csi lvgl minilibc mm td1601_evb Project_Build Project_PostBuild

All: Project_Title Project_PreBuild chip_td1601 console csi lvgl minilibc mm td1601_evb Project_Build Project_PostBuild

Project_Title:
	@echo "----------Building project:[ spi_ZC-T1D28HP-156_eez - BuildSet ]----------"

Project_PreBuild:
	@echo Executing Pre Build commands ...
	@export BOARD_PATH="E:/lierda/Jiuchen/lierda/td1601/boards/td1601_evb/v1.0" CDKPath="C:/software/C-Sky/CDK" CDK_VERSION="V2.22.0" CHIP_PATH="E:/lierda/Jiuchen/lierda/td1601/components/chips/chip_td1601/v1.0" ProjectName="spi_ZC-T1D28HP-156_eez" ProjectPath="E:/lierda/Jiuchen/lierda/td1601/projects/MoGen/lvgl/spi_ZC-T1D28HP-156_eez/" SOLUTION_PATH="E:/lierda/Jiuchen/lierda/td1601/projects/MoGen/lvgl/spi_ZC-T1D28HP-156_eez/" chip_td1601="V1.0.0" console="V1.0.0" csi="V1.0.0" lvgl="V1.0.0" minilibc="V1.0.0" mm="V1.0.0" td1601_evb="V1.0.0" && "E:/lierda/Jiuchen/lierda/td1601/projects/MoGen/lvgl/spi_ZC-T1D28HP-156_eez/utilities/pre_build.sh"
	@echo Done

chip_td1601:
	@make -r -f Obj/BuildSet/Packages/chip_td1601/V1.0.0/Makefile -j 8 -C  ./ 

console:
	@make -r -f Obj/BuildSet/Packages/console/V1.0.0/Makefile -j 8 -C  ./ 

csi:
	@make -r -f Obj/BuildSet/Packages/csi/V1.0.0/Makefile -j 8 -C  ./ 

lvgl:
	@make -r -f Obj/BuildSet/Packages/lvgl/V1.0.0/Makefile -j 8 -C  ./ 

minilibc:
	@make -r -f Obj/BuildSet/Packages/minilibc/V1.0.0/Makefile -j 8 -C  ./ 

mm:
	@make -r -f Obj/BuildSet/Packages/mm/V1.0.0/Makefile -j 8 -C  ./ 

td1601_evb:
	@make -r -f Obj/BuildSet/Packages/td1601_evb/V1.0.0/Makefile -j 8 -C  ./ 


Project_Build:
	@make -r -f spi_ZC-T1D28HP-156_eez.mk -j 8 -C  ./ 

Project_PostBuild:
	@echo Executing Post Build commands ...
	@export BOARD_PATH="E:/lierda/Jiuchen/lierda/td1601/boards/td1601_evb/v1.0" CDKPath="C:/software/C-Sky/CDK" CDK_VERSION="V2.22.0" CHIP_PATH="E:/lierda/Jiuchen/lierda/td1601/components/chips/chip_td1601/v1.0" ProjectName="spi_ZC-T1D28HP-156_eez" ProjectPath="E:/lierda/Jiuchen/lierda/td1601/projects/MoGen/lvgl/spi_ZC-T1D28HP-156_eez/" SOLUTION_PATH="E:/lierda/Jiuchen/lierda/td1601/projects/MoGen/lvgl/spi_ZC-T1D28HP-156_eez/" chip_td1601="V1.0.0" console="V1.0.0" csi="V1.0.0" lvgl="V1.0.0" minilibc="V1.0.0" mm="V1.0.0" td1601_evb="V1.0.0" && "E:/lierda/Jiuchen/lierda/td1601/projects/MoGen/lvgl/spi_ZC-T1D28HP-156_eez/utilities/aft_build.sh"
	@echo Done


clean:
	@echo "----------Cleaning project:[ spi_ZC-T1D28HP-156_eez - BuildSet ]----------"

