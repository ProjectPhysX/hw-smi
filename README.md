# hw-smi

A minimal, cross-compatible CPU/GPU telemetry monitor with accurate data directly from vendor APIs and beautiful ASCII visualization.

`moritz@opencl-pc:~/hw-smi$ sudo bin/hw-smi --bars`
<img src="https://github.com/user-attachments/assets/54f91091-a0d6-454d-8610-e415de88d796" width="100%">

`moritz@opencl-pc:~/hw-smi$ sudo bin/hw-smi --graphs`
<img src="https://github.com/user-attachments/assets/cf4c6716-e688-490d-923c-473b33f0a18d" width="100%">

## Hardware/OS Support
| Metric<br>(Windows/Linux) | CPUs<br>[Win32](https://learn.microsoft.com/de-de/windows/win32/apiindex/api-index-portal)/`/proc` | &nbsp;Nvidia&nbsp;GPUs&nbsp;<br>[NVML](https://github.com/NVIDIA/nvidia-settings/blob/main/src/nvml.h) | &nbsp;&nbsp;&nbsp;AMD&nbsp;GPUs&nbsp;&nbsp;&nbsp;<br>[ADLX](https://github.com/GPUOpen-LibrariesAndSDKs/ADLX/blob/main/SDK/Include/IPerformanceMonitoring.h)/[AMDSMI](https://github.com/ROCm/rocm-systems/blob/develop/projects/amdsmi/include/amd_smi/amdsmi.h) | &nbsp;&nbsp;Intel&nbsp;GPUs&nbsp;&nbsp;<br>[SYSMAN](https://github.com/oneapi-src/level-zero/blob/master/include/zes_api.h) |
| :---------------------  | :---------: | :---------: | :---------: | :---------: |
| device name             |   ✅/✅    |   ✅/✅    |   ✅/✅    |   ✅/✅    |
| per-core/avg usage [%]  | ✅✅/✅✅ | ✅✅/✅✅ | ✅✅/✅✅ | ✅✅/❎✅ |
| memory bandwidth [MB/s] | ❌❌/❌❌ | ✅✅/✅✅ |    🟨🟨/✅✅ | ❎❎/❎❎ |
| memory occupation [MB]  | ✅✅/✅✅ | ✅✅/✅✅ | ✅✅/✅✅ | ✅✅/✅✅ |
| temperature [°C]        |  🫠✅/✅✅ | ✅✅/✅✅ | ✅✅/✅✅ | ❎✅/❎✅  |
| power [W]               | ❌❌/❌❌ | ✅✅/✅✅ | ✅✅/✅✅ | ❎✅/✅✅ |
| fan [RPM]               | ❌❌/❌❌ |  ✅🟨/✅🟨   | ✅✅/✅🟨  |   ✅🟨/🫠🟨    |
| core clock [MHz]        |    🟨🟨/✅🟨  | ✅✅/✅✅ | ✅✅/✅✅ | ✅✅/✅✅ |
| memory clock [MHz]      | ❌❌/❌❌ |    🫠🫠/🫠🫠    | ✅✅/✅✅ | ✅✅/🫠✅  |
| PCIe bandwidth [MB/s]   |    🟨🟨/🟨🟨    | ✅✅/✅✅ | ❌❌/🫠✅  | ❎✅/❎❎ |

| Legend | Description |
| :----: | :---------- |
| AB/CD| A&nbsp;`current`&nbsp;value&nbsp;(Windows), B&nbsp;`max`&nbsp;value&nbsp;(Windows)<br>C&nbsp;`current`&nbsp;value&nbsp;(&nbsp;&nbsp;&nbsp;Linux&nbsp;&nbsp;&nbsp;), D&nbsp;`max`&nbsp;value&nbsp;(&nbsp;&nbsp;&nbsp;Linux&nbsp;&nbsp;&nbsp;) |
| ✅ | supported and working |
| ❎ | supported and working, but `administrator`/`sudo` permissions required |
|  🟨 | vendor API does not directly provide metric, but workaround/estimate/default possible |
|  🫠 | available but broken in vendor API |
| ❌ | unavailable in vendor API and no suitable default value possible |

<details><summary>API issues submitted</summary>

- https://github.com/GPUOpen-LibrariesAndSDKs/ADLX/issues/27
- https://github.com/ROCm/amdsmi/issues/182 / [183](https://github.com/ROCm/amdsmi/issues/183) / [188](https://github.com/ROCm/amdsmi/issues/188)
- https://github.com/intel/drivers.gpu.control-library/issues/120 / [146](https://github.com/intel/drivers.gpu.control-library/issues/146) / [138](https://github.com/intel/drivers.gpu.control-library/issues/138) / [149](https://github.com/intel/drivers.gpu.control-library/issues/149)
- https://github.com/oneapi-src/level-zero/issues/434 / [440](https://github.com/oneapi-src/level-zero/issues/440) / [441](https://github.com/oneapi-src/level-zero/issues/441) / [444](https://github.com/oneapi-src/level-zero/issues/444)

</details>

## Compiling the Source Code

### Windows
- Download and install [Visual Studio Community](https://visualstudio.microsoft.com/de/vs/community/). In Visual Studio Installer, add:
  - Desktop development with C++
  - MSVC v142
  - Windows 10 SDK
- [Download](https://github.com/ProjectPhysX/TaskManager/archive/refs/heads/master.zip) and unzip hw-smi.
- Open [`hw-smi.vcxproj`](hw-smi.vcxproj) in [Visual Studio Community](https://visualstudio.microsoft.com/de/vs/community/).
- Compile by clicking the <kbd>► Local Windows Debugger</kbd> button.
- Go to `hw-smi/bin/` folder and double-click `hw-smi.exe`.
- Alternatively, run from CMD:
  ```bash
  hw-smi.exe
  hw-smi.exe --bars
  hw-smi.exe --graphs
  hw-smi.exe --help
  ```
- Note that it will also work without `administrator` permissions. However, some telemetry counters on Intel GPUs are not available without `administrator` permissions.

### Linux
- Clone from GitHub:
  ```bash
  git clone https://github.com/ProjectPhysX/hw-smi.git && cd hw-smi
  ```
- Compile:
  ```bash
  chmod +x make.sh
  ./make.sh
  ```
- Run:
  ```bash
  sudo bin/hw-smi
  sudo bin/hw-smi --bars
  sudo bin/hw-smi --graphs
  sudo bin/hw-smi --help
  ```
- Note that it will also work without `sudo`. However, some telemetry counters on Intel GPUs are not available without `sudo`.

</details>
