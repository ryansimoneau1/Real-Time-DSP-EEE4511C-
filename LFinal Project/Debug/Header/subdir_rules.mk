################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Header/%.obj: ../Header/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/ti/ccs1220/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -Ooff --fp_mode=relaxed --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/lib" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/include/fpu32" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/examples/fft/2837x_rfft" --include_path="D:/ti/workspace_v10/LFinal Project" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/device_support/f2837xd/common/include" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/device_support/f2837xd/headers/include" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/driverlib/f2837xd/driverlib" --include_path="D:/ti/ccs1220/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/include" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/examples/common/f2837xd/" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/examples/common/" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/driverlib/f2837xd/driverlib/" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/include" --include_path="D:/ti/c2000/C2000Ware_3_03_00_00/libraries/dsp/FPU/c28/include/fpu32" --advice:performance=all --define=CPU1 --define=_LAUNCHXL_F28379D -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="Header/$(basename $(<F)).d_raw" --obj_directory="Header" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


