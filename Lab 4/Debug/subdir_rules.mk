################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/ti/ccs1220/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 --include_path="D:/ti/workspace_v10/Lab 4" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/device_support/f2837xd/headers/include" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/device_support/f2837xd/common/include" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/driverlib/f2837xd/driverlib" --include_path="D:/ti/ccs1220/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/include" --define=CPU1 --define=_LAUNCHXL_F28379D -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --asm_listing --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


