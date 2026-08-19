#!/usr/bin/env bash

mkdir -p bin # create directory for executable
rm -f bin/hw-smi # prevent execution of old version if compiling fails
echo_and_execute() { echo "$@"; "$@"; }

NVIDIA_LIB_A="/usr/lib/x86_64-linux-gnu/libnvidia-ml.so"
NVIDIA_LIB_B="/usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1"
NVIDIA_LIB_C="/usr/lib/aarch64-linux-gnu/libnvidia-ml.so"
NVIDIA_LIB_D="/usr/lib/aarch64-linux-gnu/libnvidia-ml.so.1"
NVIDIA_LIB_E="/usr/lib64/libnvidia-ml.so"
NVIDIA_LIB_F="/usr/lib64/libnvidia-ml.so.1"
NVIDIA_LIB_G="/usr/lib/libnvidia-ml.so"
NVIDIA_LIB_H="/usr/lib/libnvidia-ml.so.1"
NVIDIA_LIB_I="/usr/lib/wsl/lib/libnvidia-ml.so.1"
AMD_LIB_A="/usr/lib/x86_64-linux-gnu/libamd_smi.so"
AMD_LIB_B="/usr/lib/x86_64-linux-gnu/libamd_smi.so.0"
AMD_LIB_C="/usr/lib64/libamd_smi.so"
AMD_LIB_D="/usr/lib/libamd_smi.so"
AMD_LIB_E="/opt/rocm/lib/libamd_smi.so"
INTEL_LIB_A="/usr/lib/x86_64-linux-gnu/libze_intel_gpu.so.1"
INTEL_LIB_B="/usr/lib64/libze_intel_gpu.so.1"
INTEL_LIB_C="/usr/lib/libze_intel_gpu.so.1"

  if [[ -f $NVIDIA_LIB_A ]]; then NVIDIA_LIB=$NVIDIA_LIB_A;
elif [[ -f $NVIDIA_LIB_B ]]; then NVIDIA_LIB=$NVIDIA_LIB_B;
elif [[ -f $NVIDIA_LIB_C ]]; then NVIDIA_LIB=$NVIDIA_LIB_C;
elif [[ -f $NVIDIA_LIB_D ]]; then NVIDIA_LIB=$NVIDIA_LIB_D;
elif [[ -f $NVIDIA_LIB_E ]]; then NVIDIA_LIB=$NVIDIA_LIB_E;
elif [[ -f $NVIDIA_LIB_F ]]; then NVIDIA_LIB=$NVIDIA_LIB_F;
elif [[ -f $NVIDIA_LIB_G ]]; then NVIDIA_LIB=$NVIDIA_LIB_G;
elif [[ -f $NVIDIA_LIB_H ]]; then NVIDIA_LIB=$NVIDIA_LIB_H;
elif [[ -f $NVIDIA_LIB_I ]]; then NVIDIA_LIB=$NVIDIA_LIB_I; fi
  if [[ -f $AMD_LIB_A    ]]; then AMD_LIB=$AMD_LIB_A;
elif [[ -f $AMD_LIB_B    ]]; then AMD_LIB=$AMD_LIB_B;
elif [[ -f $AMD_LIB_C    ]]; then AMD_LIB=$AMD_LIB_C;
elif [[ -f $AMD_LIB_D    ]]; then AMD_LIB=$AMD_LIB_D;
elif [[ -f $AMD_LIB_E    ]]; then AMD_LIB=$AMD_LIB_E; fi
  if [[ -f $INTEL_LIB_A  ]]; then INTEL_LIB=$INTEL_LIB_A;
elif [[ -f $INTEL_LIB_B  ]]; then INTEL_LIB=$INTEL_LIB_B;
elif [[ -f $INTEL_LIB_C  ]]; then INTEL_LIB=$INTEL_LIB_C; fi

if [[ $NVIDIA_LIB ]]; then
	echo -e "\033[92mInfo\033[0m: \033[32mNvidia\033[0m GPU driver found! --> \033[32m$NVIDIA_LIB\033[0m"
else
	echo -e "\033[33mWarning\033[0m: No \033[32mNvidia\033[0m GPU driver found!"
fi

if [[ $AMD_LIB ]]; then
	echo -e "\033[92mInfo\033[0m: \033[31mAMD\033[0m GPU driver found! --> \033[31m$AMD_LIB\033[0m"
else
	echo -e "\033[33mWarning\033[0m: No \033[31mAMD\033[0m GPU driver found!"
fi

if [[ $INTEL_LIB ]]; then
	echo -e "\033[92mInfo\033[0m: \033[94mIntel\033[0m GPU driver found! --> \033[94m$INTEL_LIB\033[0m"
else
	echo -e "\033[33mWarning\033[0m: No \033[94mIntel\033[0m GPU driver found!"
fi

if [[ $NVIDIA_LIB && $AMD_LIB && $INTEL_LIB ]]; then # Nvidia+AMD+Intel GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[32mNvidia\033[0m+\033[31mAMD\033[0m+\033[94mIntel\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D NVIDIA_GPU -D AMD_GPU -D INTEL_GPU $NVIDIA_LIB $AMD_LIB $INTEL_LIB -Wl,-rpath=/opt/rocm/lib
elif [[ $NVIDIA_LIB && $AMD_LIB ]]; then # Nvidia+AMD GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[32mNvidia\033[0m+\033[31mAMD\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D NVIDIA_GPU -D AMD_GPU $NVIDIA_LIB $AMD_LIB -Wl,-rpath=/opt/rocm/lib
elif [[ $NVIDIA_LIB && $INTEL_LIB ]]; then # Nvidia+Intel GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[32mNvidia\033[0m+\033[94mIntel\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D NVIDIA_GPU -D INTEL_GPU $NVIDIA_LIB $INTEL_LIB
elif [[ $AMD_LIB && $INTEL_LIB ]]; then # AMD+Intel GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[31mAMD\033[0m+\033[94mIntel\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D AMD_GPU -D INTEL_GPU $AMD_LIB $INTEL_LIB -Wl,-rpath=/opt/rocm/lib
elif [[ $NVIDIA_LIB ]]; then # Nvidia GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[32mNvidia\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D NVIDIA_GPU $NVIDIA_LIB
elif [[ $AMD_LIB ]]; then # AMD GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[31mAMD\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D AMD_GPU $AMD_LIB -Wl,-rpath=/opt/rocm/lib
elif [[ $INTEL_LIB ]]; then # Intel GPUs
	echo -e "\033[92mInfo\033[0m: Compiling for \033[94mIntel\033[0m GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3 -D INTEL_GPU $INTEL_LIB
else # no GPUs
	echo -e "\033[92mInfo\033[0m: Compiling without GPUs:"
	echo_and_execute g++ src/main.cpp -o bin/hw-smi -std=c++17 -O3
fi

if [[ $? == 0 ]]; then
	if [[ $INTEL_LIB ]]; then # Intel SYSMAN API requires sudo permissions for some GPU counters
		echo -e "\033[92mInfo\033[0m: Compiling was successful! Run hw-smi with:\nsudo bin/hw-smi\nsudo bin/hw-smi --bars\nsudo bin/hw-smi --graphs\nsudo bin/hw-smi --log\nsudo bin/hw-smi --help"
	else
		echo -e "\033[92mInfo\033[0m: Compiling was successful! Run hw-smi with:\nbin/hw-smi\nbin/hw-smi --bars\nbin/hw-smi --graphs\nbin/hw-smi --log\nbin/hw-smi --help"
	fi
else
	echo -e "\033[91mError\033[0m: Compiling failed."
fi