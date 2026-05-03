#if defined(_WIN32)
#include "graphics.hpp"
#define WINDOWS_CPU
#define NVIDIA_GPU
#define AMD_GPU
#define INTEL_GPU
#elif defined(__linux__)||defined(__APPLE__)
#include <signal.h> // for detecting Ctrl+C
#include "utilities.hpp"
#define LINUX_CPU
volatile bool running = true;
void handler(int signo, siginfo_t* info, void* context) { // for detecting Ctrl+C
	running = false;
}
#endif // Linux

#define UPDATE_FREQUENCY 6u // update frequzency in Hz
#define GRAPH_TIME 10u // duration of plotted data in seconds
#define N (GRAPH_TIME*UPDATE_FREQUENCY)

#define STANDARD_TEXTCOLOR color_light_gray // gray
#define STANDARD_BACLGROUNDCOLOR color_black // white

struct CPU {
	string name = "";
	char vendor = (char)0;
	uint cores = 0u; // no unit
	vector<uint> usage_core_current; // in %
	uint usage_current=0u, usage_max=100u; // in %
	uint memory_current=0u, memory_max=0u, memory_pagefile=0u; // in MB
	uint temperature_current=0u, temperature_max=100u; // in 'C
	uint clock_current=0u, clock_max=5000u; // in MHz
	uint pcie_bandwidth_current=0u, pcie_bandwidth_max=0u; // accumulate bidirectional PCIe bandwidth to all GPUs in MB/s
	uint get_usage()          { return percentage(usage_current         , usage_max         ); } // in %
	uint get_memory()         { return percentage(memory_current        , memory_max        ); } // in %
	uint get_temperature()    { return percentage(temperature_current   , temperature_max   ); } // in %
	uint get_clock()          { return percentage(clock_current         , clock_max         ); } // in %
	uint get_pcie_bandwidth() { return percentage(pcie_bandwidth_current, pcie_bandwidth_max); } // in %
} cpu;

uint gpu_number = 0u;
struct GPU {
	string name = "";
	char vendor = (char)0;
	uint usage_current=0u, usage_max=100u; // in %
	uint memory_bandwidth_current=0u, memory_bandwidth_max=0u; // in MB/s
	uint memory_current=0u, memory_max=0u; // in MB
	uint temperature_current=0u, temperature_max=100u; // in 'C
	uint power_current=0u, power_max=300u; // in W
	uint fan_current=0u, fan_max=5000u; // in RPM
	uint clock_core_current=0u, clock_core_max=0u, clock_memory_current=0u, clock_memory_max=0u; // in MHz
	uint pcie_bandwidth_current=0u, pcie_bandwidth_max=0u, pcie_gen_max=0u, pcie_width_max=0u; // bidirectional PCIe bandwidth in MB/s: 32GB/s (PCIe 3.0 x16), 64GB/s (PCIe 4.0 x16), 128GB/s (PCIe 5.0 x16)
	uint memory_bus_width = 0u; // in bit
	uint memory_transfers_per_clock = 0u; // depends on memory type: 2 ((LP)DDR1-5, GDDR1-4, HBM1-4), 4 (GDDR5), 8 (GDDR5X, GDDR6), 16 (GDDR6X, GDDR6W, GDDR7)
	void set_pcie_bandwidth_max(const uint pcie_gen, const uint pcie_width) {
		pcie_gen_max = max(pcie_gen_max, pcie_gen); // harden against load-dependent reading
		pcie_width_max = max(pcie_width_max, pcie_width); // harden against load-dependent reading
		pcie_bandwidth_max = 246u*pow(2u, pcie_gen_max)*pcie_width_max;
	}
	uint get_usage()            { return percentage(usage_current           , usage_max           ); } // in %
	uint get_memory_bandwidth() { return percentage(memory_bandwidth_current, memory_bandwidth_max); } // in %
	uint get_memory()           { return percentage(memory_current          , memory_max          ); } // in %
	uint get_temperature()      { return percentage(temperature_current     , temperature_max     ); } // in %
	uint get_fan()              { return percentage(fan_current             , fan_max             ); } // in %
	uint get_power()            { return percentage(power_current           , power_max           ); } // in %
	uint get_clock_core()       { return percentage(clock_core_current      , clock_core_max      ); } // in %
	uint get_clock_memory()     { return percentage(clock_memory_current    , clock_memory_max    ); } // in %
	uint get_pcie_bandwidth()   { return percentage(pcie_bandwidth_current  , pcie_bandwidth_max  ); } // in %
};
vector<GPU> gpus;

struct Screen {
	uint width=1920u, height=1080u, fps=60u;
} screen;

vector<CircularBuffer<uchar, N>> graph_cpu_usage_core_current;
CircularBuffer<uchar, N> graph_cpu_memory;
vector<CircularBuffer<uchar, N>> graph_gpu_usage;
vector<CircularBuffer<uchar, N>> graph_gpu_memory_bandwidth;
vector<CircularBuffer<uchar, N>> graph_gpu_power;
vector<CircularBuffer<uchar, N>> graph_gpu_memory;



string clean_device_name(string name) {
	name = replace_regex(name, "\\d+th Gen\\s+", "");
	name = replace_regex(name, "\\s+\\d+-Core Processor", "");
	name = replace_regex(name, "\\s+@.*", "");
	name = replace_regex(name, "\\s+GPU \\(.*", "");
	name = replace_regex(name, "\\s*\\(R\\)", "");
	name = replace_regex(name, "\\s*\\(TM\\)", "");
	name = replace_regex(name, "\\s+", " ");
	name = replace(name, " CPU", "");
	name = replace(name, " GPU", "");
	name = replace(name, "0 Graphics", "0");
	name = replace(name, "M Graphics", "M");
	name = replace(name, "E Graphics", "E");
	name = replace(name, "A Graphics", "A");
	name = replace(name, " LP Graphics", " LP");
	name = replace(name, " Xe Graphics", " Xe");
	name = replace(name, " Xe MAX Graphics", " Xe MAX");
	name = replace(name, "HD Graphics ", "HD ");
	name = replace(name, " 6GB Laptop GPU", "M 6GB");
	name = replace(name, " 8GB Laptop GPU", "M 8GB");
	name = replace(name, " Ti Laptop GPU", "M Ti");
	name = replace(name, " Laptop GPU", "M");
	return trim(name);
}

#ifdef WINDOWS_CPU
// not available: temperature_current, temperature_max, clock_max
// broken: -
// unreliable/estimate: clock_current
#undef pi
#include <intrin.h> // for __cpuid()
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma warning(disable:4244)
#define pi 3.141592653589793
IWbemServices* wbem_services = nullptr; // connect to WMI through the IWbemLocator::ConnectServer method
FILETIME win_last_time_idle, win_last_time_kernel, win_last_time_user;
void cpu_initialize() { // initialize data
	{ // CPU
		int cpu_info[4] = { -1 };
		__cpuid(cpu_info, 0x80000000);
		const int cpu_info_0 = cpu_info[0];
		char cpu_name[48] = { '\0' };
		for(int i=0x80000002; i<=cpu_info_0; i++) {
			__cpuid(cpu_info, i);
			if(i==0x80000002) memcpy((void*)(cpu_name+ 0), (const void*)cpu_info, sizeof(cpu_info));
			if(i==0x80000003) memcpy((void*)(cpu_name+16), (const void*)cpu_info, sizeof(cpu_info));
			if(i==0x80000004) memcpy((void*)(cpu_name+32), (const void*)cpu_info, sizeof(cpu_info));
		}
		cpu.name = clean_device_name(string(cpu_name));
		if(contains(cpu.name, "Intel")) cpu.vendor = 'I';
		if(contains(cpu.name, "AMD")) cpu.vendor = 'A';
		cpu.cores = thread::hardware_concurrency();
		cpu.usage_core_current.resize(cpu.cores);
		cpu.temperature_current = cpu.temperature_max = max_uint; // no data available
		HRESULT hr; // to avoid compiler warning if return value is ignored
		hr = CoInitializeEx(0, COINIT_MULTITHREADED); // initialize COM
		hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL); // set general COM security levels
		IWbemLocator* wbem_locator = nullptr; // obtain the initial locator to WMI
		hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&wbem_locator);
		wbem_locator->ConnectServer(L"ROOT\\CIMV2", NULL, NULL, 0, NULL, 0, 0, &wbem_services); // connect to root\cimv2 namespace with current user and obtain pointer wbem_services to make IWbemServices calls
		wbem_locator->Release();
	} { // RAM
		GetSystemTimes(&win_last_time_idle, &win_last_time_kernel, &win_last_time_user); // initialize
	} { // screen
		MONITORINFO mi ={ sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(GetConsoleWindow(), MONITOR_DEFAULTTONEAREST), &mi);
		screen.width  = (uint)(mi.rcMonitor.right-mi.rcMonitor.left); // get screen size
		screen.height = (uint)(mi.rcMonitor.bottom-mi.rcMonitor.top);
		DEVMODE dm; // get screen fps
		memset(&dm, 0, sizeof(DEVMODE));
		if(EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)!=0) screen.fps = (uint)dm.dmDisplayFrequency;
	}
}
void cpu_update() {
	{ // get CPU utilization
		FILETIME time_idle, time_kernel, time_user;
		GetSystemTimes(&time_idle, &time_kernel, &time_user);
		const double d_cpu_idl = time_idle.dwLowDateTime-win_last_time_idle.dwLowDateTime;
		const double d_cpu_ker = time_kernel.dwLowDateTime-win_last_time_kernel.dwLowDateTime;
		const double d_cpu_usr = time_user.dwLowDateTime-win_last_time_user.dwLowDateTime;
		const double d_cpu_sys = d_cpu_ker+d_cpu_usr;
		win_last_time_idle = time_idle;
		win_last_time_kernel = time_kernel;
		win_last_time_user = time_user;
		const double cpu_usage_instant = 100.0*(d_cpu_sys-d_cpu_idl)/d_cpu_sys;
		cpu.usage_current = to_uint(cpu_usage_instant);
	} { // get CPU core utilization
		IEnumWbemClassObject* wbem_enumerator = nullptr; // use the IWbemServices pointer to make requests of WMI
		wbem_services->ExecQuery(L"WQL", L"SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor", WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &wbem_enumerator);
		while(wbem_enumerator) {
			ULONG u_return = 0ul;
			IWbemClassObject* wbem_class_object = nullptr; // get the data from the query
			wbem_enumerator->Next(WBEM_INFINITE, 1ul, &wbem_class_object, &u_return);
			if(u_return==0ul) break;
			VARIANT wbem_variant = {};
			wbem_class_object->Get(L"Name", 0l, &wbem_variant, 0l, 0l); // get value of the "Name" property
			const std::wstring wcore = wbem_variant.bstrVal;
			wbem_class_object->Get(L"PercentProcessorTime", 0l, &wbem_variant, 0l, 0l); // get value of the "PercentProcessorTime" property
			const std::wstring wusage = wbem_variant.bstrVal;
			wbem_class_object->Release(); // avoid memory leak
			const string core=string(wcore.begin(), wcore.end()), usage=string(wusage.begin(), wusage.end());
			if(parse_sanity_check(trim(core), "\\+?\\d+")) cpu.usage_core_current[to_uint(core, 0u)] = to_uint(usage, 0u);
		}
		wbem_enumerator->Release(); // avoid memory leak
	} { // RAM
		MEMORYSTATUSEX memory_status = {};
		memory_status.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memory_status);
		cpu.memory_current = (uint)((524288ull+memory_status.ullTotalPhys-memory_status.ullAvailPhys)/1048576ull);
		cpu.memory_max = (uint)((memory_status.ullTotalPhys+524288ull)/1048576ull);
		cpu.memory_pagefile = (uint)((memory_status.ullTotalPageFile+524288ull)/1048576ull);
	} { // get CPU clock frequency (not working)
		LARGE_INTEGER qwWait, qwStart, qwCurrent;
		QueryPerformanceCounter(&qwStart);
		QueryPerformanceFrequency(&qwWait);
		qwWait.QuadPart >>= 5;
		unsigned __int64 start = __rdtsc();
		do {
			QueryPerformanceCounter(&qwCurrent);
		} while(qwCurrent.QuadPart-qwStart.QuadPart<qwWait.QuadPart);
		cpu.clock_current = to_uint((float)((__rdtsc()-start)<<5)*1.0E-6f);
	} { // accumulate PCIe bandwidth to all GPUs
		uint cpu_pcie_bandwidth_current = 0u;
		uint cpu_pcie_bandwidth_max = 0u;
		for(uint g=0u; g<gpu_number; g++) {
			cpu_pcie_bandwidth_current += gpus[g].pcie_bandwidth_current<max_uint ? gpus[g].pcie_bandwidth_current : 0u;
			cpu_pcie_bandwidth_max += gpus[g].pcie_bandwidth_max<max_uint ? gpus[g].pcie_bandwidth_max : 0u;
		}
		cpu.pcie_bandwidth_current = cpu_pcie_bandwidth_current;
		cpu.pcie_bandwidth_max = cpu_pcie_bandwidth_max;
	}
}
void cpu_finalize() {
	wbem_services->Release();
	CoUninitialize();
}
#endif // WINDOWS_CPU

#ifdef LINUX_CPU
// not available: temperature_max, clock_max
// broken: -
// unreliable/estimate: -
ulong lx_last_cpu_time_usage = 0ull;
ulong lx_last_cpu_time_idle = 0ull;
vector<ulong> lx_last_cpu_times_usage;
vector<ulong> lx_last_cpu_times_idle;
void cpu_initialize() { // initialize data
	cpu.cores = thread::hardware_concurrency();
	cpu.usage_core_current.resize(cpu.cores);
	lx_last_cpu_times_usage.resize(cpu.cores);
	lx_last_cpu_times_idle.resize(cpu.cores);
	{ // CPU name, read /proc/cpuinfo
		const string proc_cpuinfo = read_file("/proc/cpuinfo");
		if(length(proc_cpuinfo)>0u) {
			const vector<string> lines = split_regex(proc_cpuinfo, "\\s*\n\\s*");
			for(uint i=0u; i<(uint)lines.size(); i++) {
				const vector<string> values = split_regex(lines[i], "\\s*:\\s*");
				bool found = false;
				if(values[0]=="model name") {
					string cpu_name = clean_device_name(values[1]);
					cpu.name = length(cpu_name)>0u ? cpu_name : "?";
					if(contains(cpu.name, "Intel")) cpu.vendor = 'I';
					if(contains(cpu.name, "AMD")) cpu.vendor = 'A';
					if(contains(cpu.name, "Neoverse")) cpu.vendor = 'N';
					found = true;
					break;
				}
				if(!found) cpu.name = "?"; // no data available
			}
		} else {
			cpu.name = "?"; // no data available
		}
	} { // CPU, read /proc/stat
		const string proc_stat = read_file("/proc/stat");
		if(length(proc_stat)>0u) {
			const vector<string> lines = split_regex(proc_stat, "\\s*\n\\s*");
			if((uint)lines.size()>=1u) {
				const vector<string> values = split_regex(lines[0]);
				if((uint)values.size()>4u) {
					lx_last_cpu_time_usage = to_ulong(values[1])+to_ulong(values[2])+to_ulong(values[3]);
					lx_last_cpu_time_idle = to_ulong(values[4]);
				}
			}
			for(uint i=1u; i<min((uint)lines.size(), cpu.cores)+1u; i++) {
				const vector<string> values = split_regex(lines[i]);
				if((uint)values.size()>4u) {
					lx_last_cpu_times_usage[i-1u] = to_ulong(values[1])+to_ulong(values[2])+to_ulong(values[3]);
					lx_last_cpu_times_idle[i-1u] = to_ulong(values[4]);
				}
			}
		}
	}
}
void cpu_update() {
	{ // CPU, read /proc/stat, https://www.idnt.net/en-US/kb/941772
		const string proc_stat = read_file("/proc/stat");
		if(length(proc_stat)>0u) {
			ulong lx_cpu_time_usage = 0ull;
			ulong lx_cpu_time_idle = 0ull;
			vector<ulong> lx_cpu_times_usage(cpu.cores);
			vector<ulong> lx_cpu_times_idle(cpu.cores);
			const vector<string> lines = split_regex(proc_stat, "\\s*\n\\s*");
			if((uint)lines.size()>=1u) {
				const vector<string> values = split_regex(lines[0]);
				if((uint)values.size()>4u) {
					lx_cpu_time_usage = to_ulong(values[1])+to_ulong(values[2])+to_ulong(values[3]);
					lx_cpu_time_idle = to_ulong(values[4]);
				}
			}
			for(uint i=1u; i<1u+min((uint)lines.size(), cpu.cores); i++) {
				const vector<string> values = split_regex(lines[i]);
				if((uint)values.size()>4u) {
					lx_cpu_times_usage[i-1u] = to_ulong(values[1])+to_ulong(values[2])+to_ulong(values[3]);
					lx_cpu_times_idle[i-1u] = to_ulong(values[4]);
				}
			}
			const ulong cpu_usage_interval = lx_cpu_time_usage-lx_last_cpu_time_usage+lx_cpu_time_idle-lx_last_cpu_time_idle;
			cpu.usage_current = cpu_usage_interval>0ull ? (uint)((100ull*(lx_cpu_time_usage-lx_last_cpu_time_usage)+cpu_usage_interval/2ull)/cpu_usage_interval) : cpu.usage_current; // reuse last value if interval is 0
			lx_last_cpu_time_usage = lx_cpu_time_usage;
			lx_last_cpu_time_idle = lx_cpu_time_idle;
			for(uint i=0u; i<cpu.cores; i++) {
				const ulong cpu_usage_core_interval = lx_cpu_times_usage[i]-lx_last_cpu_times_usage[i]+lx_cpu_times_idle[i]-lx_last_cpu_times_idle[i];
				cpu.usage_core_current[i] = cpu_usage_core_interval>0ull ? (uint)((100ull*(lx_cpu_times_usage[i]-lx_last_cpu_times_usage[i])+cpu_usage_core_interval/2ull)/cpu_usage_core_interval) : cpu.usage_core_current[i]; // reuse last value if interval is 0
				lx_last_cpu_times_usage[i] = lx_cpu_times_usage[i];
				lx_last_cpu_times_idle[i] = lx_cpu_times_idle[i];
			}
		} else {
			cpu.usage_current = cpu.usage_max = max_uint; // no data available
			for(uint i=0u; i<cpu.cores; i++) {
				cpu.usage_core_current[i] = max_uint; // no data available
			}
		}
	} { // CPU clock, read /proc/cpuinfo
		const string proc_cpuinfo = read_file("/proc/cpuinfo");
		if(length(proc_cpuinfo)>0u) {
			float cpu_clock_current = 0.0f;
			uint counter = 0u;
			size_t pos_cpu_mhz_1 = 0;
			while(true) {
				pos_cpu_mhz_1 = proc_cpuinfo.find("cpu MHz", pos_cpu_mhz_1);
				const size_t pos_cpu_mhz_2 = proc_cpuinfo.find("\n", pos_cpu_mhz_1);
				if(pos_cpu_mhz_1!=string::npos) {
					pos_cpu_mhz_1 = proc_cpuinfo.find(":", pos_cpu_mhz_1+7);
					const string cpu_mhz = trim(proc_cpuinfo.substr(pos_cpu_mhz_1+1, pos_cpu_mhz_2-(pos_cpu_mhz_1+1)));
					cpu_clock_current += to_float(cpu_mhz);
					counter++;
				} else {
					break;
				}
			}
			cpu_clock_current /= (float)counter;
			cpu.clock_current = cpu_clock_current;
		} else {
			cpu.clock_current = cpu.clock_max = max_uint; // no data available
		}
	} { // CPU temperature, read /sys/class/thermal/thermal_zone1/temp (x86_pkg_temp)
		const string proc_temp = read_file("/sys/class/thermal/thermal_zone1/temp");
		if(length(proc_temp)>0u) {
			const uint cpu_temperature_current = (to_uint(trim(proc_temp))+500u)/1000u;
			cpu.temperature_current = cpu_temperature_current>0u ? cpu_temperature_current : cpu.temperature_current; // harden against reading dropouts
			cpu.temperature_max = cpu.temperature_current>0u ? 100u : 0u; // make 0 if no data is available
		} else {
			cpu.temperature_current = cpu.temperature_max = max_uint; // no data available
		}
	} { // RAM, read /proc/meminfo
		const string proc_meminfo = read_file("/proc/meminfo");
		if(length(proc_meminfo)>0u) {
			const size_t pos_mem_total_1 = proc_meminfo.find("MemTotal:", 0);
			const size_t pos_mem_total_2 = proc_meminfo.find("kB", pos_mem_total_1+9);
			const string mem_total = trim(proc_meminfo.substr(pos_mem_total_1+9, pos_mem_total_2-(pos_mem_total_1+9)));
			cpu.memory_max = (uint)((to_ulong(mem_total)+512ull)/1024ull);
			const size_t pos_mem_available_1 = proc_meminfo.find("MemAvailable:", 0);
			const size_t pos_mem_available_2 = proc_meminfo.find("kB", pos_mem_available_1+13);
			if(pos_mem_available_1!=string::npos) {
				const string mem_available = trim(proc_meminfo.substr(pos_mem_available_1+13, pos_mem_available_2-(pos_mem_available_1+13)));
				cpu.memory_current = cpu.memory_max-(uint)((to_ulong(mem_available)+512ull)/1024ull);
			} else {
				const size_t pos_mem_free_1 = proc_meminfo.find("MemFree:", 0);
				const size_t pos_mem_free_2 = proc_meminfo.find("kB", pos_mem_free_1+8);
				const string mem_free = trim(proc_meminfo.substr(pos_mem_free_1+8, pos_mem_free_2-(pos_mem_free_1+8)));
				cpu.memory_current = cpu.memory_max-(uint)((to_ulong(mem_free)+512ull)/1024ull); // harden against unavailable MemAvailable counter
			}
		} else {
			cpu.memory_current = cpu.memory_max = max_uint; // no data available
		}
	} { // accumulate PCIe bandwidth to all GPUs
		uint cpu_pcie_bandwidth_current = 0u;
		uint cpu_pcie_bandwidth_max = 0u;
		for(uint g=0u; g<gpu_number; g++) {
			cpu_pcie_bandwidth_current += gpus[g].pcie_bandwidth_current<max_uint ? gpus[g].pcie_bandwidth_current : 0u;
			cpu_pcie_bandwidth_max += gpus[g].pcie_bandwidth_max<max_uint ? gpus[g].pcie_bandwidth_max : 0u;
		}
		cpu.pcie_bandwidth_current = cpu_pcie_bandwidth_current;
		cpu.pcie_bandwidth_max = cpu_pcie_bandwidth_max;
	}
}
void cpu_finalize() {
}
#endif // LINUX_CPU

#ifdef NVIDIA_GPU
// not available: fan_max, memory_transfers_per_clock
// broken: power_current (pre-Pascal), fan_current (pre-Pascal)
// unreliable/estimate: fan_current
#include "NVML/include/nvml.h" // https://github.com/NVIDIA/nvidia-settings/blob/main/src/nvml.h
#pragma warning(disable:26812)
uint nvml_gpu_start=0u, nvml_gpu_number=0u;
vector<nvmlDevice_t> nvml_devices;
void gpu_initialize_nvidia() {
#ifdef _WIN32 // return if nvml.dll is not available
	if(!LoadLibrary("nvml.dll")) return;
#endif // Windows
	if(nvmlInit()!=NVML_SUCCESS) return;
	nvmlDeviceGetCount(&nvml_gpu_number);
	nvml_devices.resize(nvml_gpu_number);
	for(uint i=0u; i<nvml_gpu_number; i++) nvmlDeviceGetHandleByIndex(i, &nvml_devices[i]);
	nvml_gpu_start = gpu_number;
	gpu_number += nvml_gpu_number;
	gpus.resize(gpu_number);
	for(uint i=0u; i<(uint)nvml_devices.size(); i++) {
		const uint g = nvml_gpu_start+i;
		const nvmlDevice_t& nvml_device = nvml_devices[i];
		char char_gpu_name[96];
		uint gpu_memory_transfers_max_half = 0u;
		nvmlDeviceGetName(nvml_device, char_gpu_name, 96u);
		nvmlDeviceGetMaxClockInfo(nvml_device, NVML_CLOCK_SM, &gpus[g].clock_core_max);
		nvmlDeviceGetMaxClockInfo(nvml_device, NVML_CLOCK_MEM, &gpu_memory_transfers_max_half); // wrongly reports (MT/s / 2) instead of MHz
		nvmlDeviceGetMemoryBusWidth(nvml_device, &gpus[g].memory_bus_width);
		gpus[g].name = clean_device_name(string(char_gpu_name));
		gpus[g].vendor = 'N'; // Nvidia
		gpus[g].memory_transfers_per_clock = 8u; // no data available
		gpus[g].memory_bandwidth_max = (gpu_memory_transfers_max_half*2u)*gpus[g].memory_bus_width/8u;
		gpus[g].clock_memory_max = (gpu_memory_transfers_max_half*2u)/gpus[g].memory_transfers_per_clock;
	}
}
void gpu_update_nvidia() {
	for(uint i=0u; i<(uint)nvml_devices.size(); i++) {
		const uint g = nvml_gpu_start+i;
		const nvmlDevice_t& nvml_device = nvml_devices[i];
		nvmlUtilization_t nvml_utilization = {};
		nvmlMemory_t nvml_memory = {};
		nvmlFanSpeedInfo_t nvml_fan = {};
		uint gpu_temperature_current=0u, gpu_milliwatts_current=0u, gpu_milliwatts_total=0u, gpu_fan_percent=0u,gpu_memory_transfers_current_half=0u, gpu_pcie_bandwidth_tx_kbps=0u, gpu_pcie_bandwidth_rx_kbps=0u, gpu_pcie_gen=0u, gpu_pcie_width=0u;
		nvmlDeviceGetUtilizationRates(nvml_device, &nvml_utilization);
		nvmlDeviceGetMemoryInfo(nvml_device, &nvml_memory);
		nvmlDeviceGetTemperature(nvml_device, NVML_TEMPERATURE_GPU, &gpu_temperature_current);
		const bool support_power = (NVML_SUCCESS==nvmlDeviceGetPowerUsage(nvml_device, &gpu_milliwatts_current));
		nvmlDeviceGetEnforcedPowerLimit(nvml_device, &gpu_milliwatts_total);
		const bool support_fan_rpm = (NVML_SUCCESS==nvmlDeviceGetFanSpeedRPM(nvml_device, &nvml_fan)); // in RPM
		const bool support_fan_percent = (NVML_SUCCESS==nvmlDeviceGetFanSpeed(nvml_device, &gpu_fan_percent)); // in %
		nvmlDeviceGetClockInfo(nvml_device, NVML_CLOCK_SM, &gpus[g].clock_core_current);
		nvmlDeviceGetClockInfo(nvml_device, NVML_CLOCK_MEM, &gpu_memory_transfers_current_half); // wrongly reports (MT/s / 2) instead of MHz
		nvmlDeviceGetPcieThroughput(nvml_device, NVML_PCIE_UTIL_TX_BYTES, &gpu_pcie_bandwidth_tx_kbps); // transmit
		nvmlDeviceGetPcieThroughput(nvml_device, NVML_PCIE_UTIL_RX_BYTES, &gpu_pcie_bandwidth_rx_kbps); // receive
		nvmlDeviceGetCurrPcieLinkGeneration(nvml_device, &gpu_pcie_gen);
		nvmlDeviceGetCurrPcieLinkWidth(nvml_device, &gpu_pcie_width);
		gpus[g].usage_current = (uint)nvml_utilization.gpu;
		gpus[g].memory_bandwidth_current = (((gpu_memory_transfers_current_half*2u)*gpus[g].memory_bus_width/8u)*(uint)nvml_utilization.memory+50u)/100u; // +50u for correct rounding
		gpus[g].memory_current = (uint)((nvml_memory.used+524288ull)/1048576ull);
		gpus[g].memory_max = max(gpus[g].memory_max, (uint)((nvml_memory.total+524288ull)/1048576ull)); // harden against reading dropouts
		gpus[g].temperature_current = gpu_temperature_current>0u ? gpu_temperature_current : gpus[g].temperature_current; // harden against reading dropouts
		gpus[g].temperature_max = gpus[g].temperature_current>0u ? 100u : 0u; // make 0 if no data is available
		gpus[g].power_current = support_power ? (gpu_milliwatts_current+500u)/1000u : max_uint;
		gpus[g].power_max = gpu_milliwatts_total>0u ? (gpu_milliwatts_total+500u)/1000u : gpus[g].power_max; // harden against reading dropouts
		gpus[g].fan_current = support_fan_rpm ? nvml_fan.speed : support_fan_percent ? (gpus[g].fan_max*gpu_fan_percent+50u)/100u : max_uint; // harden against broken counters
		gpus[g].fan_max = support_fan_rpm||support_fan_percent ? 5000u : max_uint; // no data available
		gpus[g].clock_memory_current = (gpu_memory_transfers_current_half*2u)/gpus[g].memory_transfers_per_clock;
		const uint gpu_pcie_bandwidth_current = (gpu_pcie_bandwidth_tx_kbps+gpu_pcie_bandwidth_rx_kbps+512u)/1024u;
		gpus[g].set_pcie_bandwidth_max(gpu_pcie_gen, gpu_pcie_width); // harden against load-dependent reading
		gpus[g].pcie_bandwidth_current = gpu_pcie_bandwidth_current<2u*gpus[g].pcie_bandwidth_max ? gpu_pcie_bandwidth_current : 0u; // harden against spikes
	}
}
void gpu_finalize_nvidia() {
	nvml_devices.clear();
	nvmlShutdown();
}
#endif // NVIDIA_GPU

#ifdef AMD_GPU
#if defined(_WIN32)
// not available: pcie_bandwidth_current, pcie_bandwidth_max, memory_transfers_per_clock
// broken: -
// unreliable/estimate: memory_bandwidth_current, memory_bandwidth_max, memory_bus_width
#include "ADLX/include/ADLXHelper.h" // https://github.com/GPUOpen-LibrariesAndSDKs/ADLX/blob/main/SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h
#include "ADLX/include/IPerformanceMonitoring.h" // https://github.com/GPUOpen-LibrariesAndSDKs/ADLX/blob/main/SDK/Include/IPerformanceMonitoring.h
#pragma warning(disable:26812)
uint adlx_gpu_start=0u, adlx_gpu_number=0u;
static ADLXHelper adlx_helper; // https://github.com/GPUOpen-LibrariesAndSDKs/ADLX/blob/main/Samples/CPP/PerformanceMonitoring/PerfGPUMetrics/mainPerfGPUMetrics.cpp
adlx::IADLXPerformanceMonitoringServicesPtr adlx_pms;
adlx::IADLXGPUListPtr adlx_list;
uint adlx_get_gpu_bandwidth_max(const string& device_id, const uint gpu_vram, const uint gpu_clock_memory_max) { // https://drmdb.emersion.fr/devices?driver=amdgpu
	/**/ if(device_id=="6819"&&gpu_vram== 2u) return  179200u; // AMD Radeon R7 270 1024SP
	else if(device_id=="6810"&&gpu_vram== 2u) return  179200u; // AMD Radeon R7 270(X)/370(X)
	else if(device_id=="679A"&&gpu_vram== 3u) return  240000u; // AMD Radeon R9 280
	else if(device_id=="6798"&&gpu_vram== 3u) return  288000u; // AMD Radeon R9 280X
	else if(device_id=="67B1"&&gpu_vram== 4u) return  320000u; // AMD Radeon R9 290
	else if(device_id=="6938"&&gpu_vram== 4u) return  182400u; // AMD Radeon R9 380(X)
	else if(device_id=="67B1"&&gpu_vram== 8u) return  384000u; // AMD Radeon R9 390
	else if(device_id=="7300"&&gpu_vram== 4u) return  512000u; // AMD Radeon R9 Fury (X)
	else if(device_id=="67EF"&&gpu_vram== 4u) return  112000u; // AMD Radeon RX 460/560
	else if(device_id=="67FF"&&gpu_vram== 4u) return  112000u; // AMD Radeon RX 560(X)
	else if(device_id=="67DF"&&gpu_vram== 4u) return  224000u; // AMD Radeon RX 470/570/580 4GB
	else if(device_id=="67DF"&&gpu_vram== 8u) return  256000u; // AMD Radeon RX 470/480/570/580/590 8GB
	else if(device_id=="6FDF"&&gpu_vram== 8u) return  256000u; // AMD Radeon RX 580 2048SP
	else if(device_id=="694E"&&gpu_vram== 4u) return  179200u; // AMD Radeon RX Vega M GL
	else if(device_id=="694C"&&gpu_vram== 4u) return  204800u; // AMD Radeon RX Vega M GH
	else if(device_id=="687F"&&gpu_vram== 8u) return  483800u; // AMD Radeon RX Vega 56/64
	else if(device_id=="687F"&&gpu_vram==16u) return  483800u; // AMD Radeon RX Vega Frontier Edition
	else if(device_id=="66AF"&&gpu_vram==16u) return 1024000u; // AMD Radeon VII (Pro)
	else if(device_id=="7340"&&gpu_vram== 4u) return  224000u; // AMD Radeon RX 5500
	else if(device_id=="7340"&&gpu_vram== 8u) return  224000u; // AMD Radeon RX 5500 XT
	else if(device_id=="731F"&&gpu_vram== 6u) return  288000u; // AMD Radeon RX 5600 (XT)
	else if(device_id=="731F"&&gpu_vram== 8u) return  448000u; // AMD Radeon RX 5700 (XT)
	else if(device_id=="73BF"&&gpu_vram==16u) return  512000u; // AMD Radeon RX 6800/6900 (XT)
	else if(device_id=="73AF"&&gpu_vram==16u) return  512000u; // AMD Radeon RX 6900 XT
	else if(device_id=="73A5"&&gpu_vram==16u) return  576000u; // AMD Radeon RX 6950 XT
	else if(device_id=="7480"&&gpu_vram== 8u) return  288000u; // AMD Radeon RX 7600 / Pro W7600
	else if(device_id=="7480"&&gpu_vram==16u) return  288000u; // AMD Radeon RX 7600 XT
	else if(device_id=="747E"&&gpu_vram==12u) return  432000u; // AMD Radeon RX 7700 XT
	else if(device_id=="747E"&&gpu_vram==16u) return  624000u; // AMD Radeon RX 7800 XT
	else if(device_id=="744C"&&gpu_vram==16u) return  576000u; // AMD Radeon RX 7900 GRE
	else if(device_id=="744C"&&gpu_vram==20u) return  800000u; // AMD Radeon RX 7900 XT
	else if(device_id=="744C"&&gpu_vram==24u) return  960000u; // AMD Radeon RX 7900 XTX
	else if(device_id=="744C"&&gpu_vram==32u) return  576000u; // AMD Radeon Pro W7800
	else if(device_id=="744C"&&gpu_vram==48u) return  864000u; // AMD Radeon Pro W7900
	else if(device_id=="7590"&&gpu_vram==16u) return  320000u; // AMD Radeon RX 9060 XT
	else if(device_id=="7550"&&gpu_vram==16u) return  640000u; // AMD Radeon RX 9070 (XT)
	else return (gpu_vram*16u)*gpu_clock_memory_max; // fallback: gpu_memory_bus_width*gpu_clock_memory_max
}
void gpu_initialize_amd() {
	if(adlx_helper.Initialize()!=ADLX_OK) return;
	adlx_helper.GetSystemServices()->GetPerformanceMonitoringServices(&adlx_pms);
	adlx_helper.GetSystemServices()->GetGPUs(&adlx_list);
	adlx_gpu_start = gpu_number;
	adlx_gpu_number = (uint)adlx_list->Size();
	gpu_number += adlx_gpu_number;
	gpus.resize(gpu_number);
	for(uint i=0u; i<adlx_gpu_number; i++) {
		const uint g = adlx_gpu_start+i;
		adlx::IADLXGPUPtr adlx_gpu;
		adlx::IADLXGPUMetricsSupportPtr adlx_gpu_metrics_support;
		adlx_list->At(adlx_list->Begin()+i, &adlx_gpu);
		adlx_pms->GetSupportedGPUMetrics(adlx_gpu, &adlx_gpu_metrics_support);
		const char* gpu_name = nullptr;
		const char* device_id = nullptr;
		int gpu_memory_min=0, gpu_memory_max=0;
		int gpu_power_min=0, gpu_power_max=0;
		int gpu_fan_min=0, gpu_fan_max=0;
		int gpu_clock_core_min=0, gpu_clock_core_max=0;
		int gpu_clock_memory_min=0, gpu_clock_memory_max=0;
		adlx_gpu->Name(&gpu_name);
		adlx_gpu->DeviceId(&device_id);
		adlx_gpu_metrics_support->GetGPUVRAMRange(&gpu_memory_min, &gpu_memory_max); // in MB
		adlx_gpu_metrics_support->GetGPUTotalBoardPowerRange(&gpu_power_min, &gpu_power_max); // in W
		adlx_gpu_metrics_support->GetGPUFanSpeedRange(&gpu_fan_min, &gpu_fan_max); // in RPM
		adlx_gpu_metrics_support->GetGPUClockSpeedRange(&gpu_clock_core_min, &gpu_clock_core_max); // in MHz
		adlx_gpu_metrics_support->GetGPUVRAMClockSpeedRange(&gpu_clock_memory_min, &gpu_clock_memory_max); // in MHz
		gpus[g].name = clean_device_name(string(gpu_name));
		gpus[g].vendor = 'A'; // AMD
		gpus[g].memory_bandwidth_max = adlx_get_gpu_bandwidth_max(trim(string(device_id)), (((uint)gpu_memory_max+500u)/1000u), (uint)gpu_clock_memory_max); // harden against unavailable counter
		gpus[g].memory_max = (uint)gpu_memory_max;
		gpus[g].power_max = (uint)gpu_power_max;
		gpus[g].fan_max = gpu_fan_max>0 ? (uint)gpu_fan_max : 5000u;
		gpus[g].memory_bus_width = (((uint)gpu_memory_max+500u)/1000u)*16u; // estimate
		gpus[g].memory_transfers_per_clock = 8u; // no data available
		gpus[g].clock_core_max = min((uint)gpu_clock_core_max, 3000u); // harden against broken counters
		gpus[g].clock_memory_max = min((uint)gpu_clock_memory_max, gpus[g].memory_bus_width>0u ? gpus[g].memory_bandwidth_max*8u/(gpus[g].memory_transfers_per_clock*gpus[g].memory_bus_width) : 0u); // harden against broken counters
		gpus[g].pcie_bandwidth_current = gpus[g].pcie_bandwidth_max = max_uint; // no data available
	}
}
void gpu_update_amd() {
	for(uint i=0u; i<adlx_gpu_number; i++) {
		const uint g = adlx_gpu_start+i;
		adlx::IADLXGPUPtr adlx_gpu;
		adlx::IADLXGPUMetricsPtr adlx_gpu_metrics;
		adlx_list->At(adlx_list->Begin()+i, &adlx_gpu);
		adlx_pms->GetCurrentGPUMetrics(adlx_gpu, &adlx_gpu_metrics);
		double gpu_usage=0.0, gpu_temperature_current=0.0, gpu_power_current=0.0;
		int gpu_memory_current=0, gpu_fan_current=0, gpu_clock_core_current=0, gpu_clock_memory_current=0;
		adlx_gpu_metrics->GPUUsage(&gpu_usage); // in %
		adlx_gpu_metrics->GPUVRAM(&gpu_memory_current); // in MB
		adlx_gpu_metrics->GPUTemperature(&gpu_temperature_current); // in 'C
		adlx_gpu_metrics->GPUTotalBoardPower(&gpu_power_current); // in W
		adlx_gpu_metrics->GPUFanSpeed(&gpu_fan_current); // in RPM
		adlx_gpu_metrics->GPUClockSpeed(&gpu_clock_core_current); // in MHz
		adlx_gpu_metrics->GPUVRAMClockSpeed(&gpu_clock_memory_current); // in MHz
		gpus[g].usage_current = to_uint(gpu_usage);
		gpus[g].memory_current = (uint)gpu_memory_current;
		gpus[g].temperature_current = gpu_temperature_current>0.0 ? to_uint(gpu_temperature_current) : gpus[g].temperature_current; // harden against reading dropouts
		gpus[g].temperature_max = gpus[g].temperature_current>0u ? 100u : 0u; // make 0 if no data is available
		gpus[g].power_current = to_uint(gpu_power_current);
		gpus[g].fan_current = (uint)gpu_fan_current;
		gpus[g].clock_core_current = (uint)gpu_clock_core_current;
		gpus[g].clock_memory_current = (uint)gpu_clock_memory_current;
		gpus[g].memory_bandwidth_current = gpus[g].clock_memory_max>0u ? gpus[g].memory_bandwidth_max*to_uint(gpu_usage/100.0)*(uint)gpu_clock_memory_current/gpus[g].clock_memory_max : 0u; // estimate VRAM bandwidth via GPU usage and memory clock
	}
}
void gpu_finalize_amd() {
	adlx_list.Release();
	adlx_helper.Terminate();
}
#elif defined(__linux__)
// not available: fan_max
// broken: pcie_bandwidth_current
// unreliable/estimate: -
#include "AMDSMI/include/amdsmi.h" // https://github.com/ROCm/amdsmi/blob/amd-mainline/include/amd_smi/amdsmi.h https://rocm.docs.amd.com/projects/amdsmi/en/latest/how-to/amdsmi-cpp-lib.html
uint amdsmi_gpu_start=0u, amdsmi_gpu_number=0u;
vector<amdsmi_processor_handle> amdsmi_devices;
void gpu_initialize_amd() {
	if(amdsmi_init(AMDSMI_INIT_AMD_GPUS)!=AMDSMI_STATUS_SUCCESS) return;
	uint amdsmi_socket_count = 0u;
	amdsmi_get_socket_handles(&amdsmi_socket_count, nullptr);
	vector<amdsmi_socket_handle> amdsmi_sockets(amdsmi_socket_count);
	amdsmi_get_socket_handles(&amdsmi_socket_count, &amdsmi_sockets[0]);
	for(const amdsmi_socket_handle& amdsmi_socket : amdsmi_sockets) {
		uint32_t amdsmi_device_count = 0;
		amdsmi_get_processor_handles(amdsmi_socket, &amdsmi_device_count, nullptr);
		vector<amdsmi_processor_handle> amdsmi_devices_to_add(amdsmi_device_count);
		amdsmi_get_processor_handles(amdsmi_socket, &amdsmi_device_count, &amdsmi_devices_to_add[0]);
		for(const amdsmi_processor_handle& amdsmi_device_to_add : amdsmi_devices_to_add) {
			processor_type_t amdsmi_processor_type = {};
			amdsmi_get_processor_type(amdsmi_device_to_add, &amdsmi_processor_type);
			if(amdsmi_processor_type==AMDSMI_PROCESSOR_TYPE_AMD_GPU) amdsmi_devices.push_back(amdsmi_device_to_add);
		}
	}
	amdsmi_gpu_start = gpu_number;
	amdsmi_gpu_number = (uint)amdsmi_devices.size();
	gpu_number += amdsmi_gpu_number;
	gpus.resize(gpu_number);
	for(uint i=0u; i<(uint)amdsmi_devices.size(); i++) {
		const uint g = amdsmi_gpu_start+i;
		const amdsmi_processor_handle& amdsmi_device = amdsmi_devices[i];
		amdsmi_asic_info_t amdsmi_asic_info = {};
		amdsmi_vram_info_t amdsmi_vram_info = {};
		amdsmi_frequencies_t amdsmi_frequencies_core = {};
		amdsmi_frequencies_t amdsmi_frequencies_memory = {};
		amdsmi_get_gpu_asic_info(amdsmi_device, &amdsmi_asic_info);
		amdsmi_get_gpu_vram_info(amdsmi_device, &amdsmi_vram_info);
		amdsmi_get_clk_freq(amdsmi_device, AMDSMI_CLK_TYPE_GFX, &amdsmi_frequencies_core); // in Hz
		amdsmi_get_clk_freq(amdsmi_device, AMDSMI_CLK_TYPE_MEM, &amdsmi_frequencies_memory); // in Hz
		gpus[g].name = clean_device_name(string(amdsmi_asic_info.market_name));
		gpus[g].vendor = 'A';
		gpus[g].fan_max = 5000u; // no data available
		gpus[g].clock_core_max = 0u;
		gpus[g].clock_memory_max = 0u;
		for(uint f=0u; f<amdsmi_frequencies_core.num_supported; f++) gpus[g].clock_core_max = max(gpus[g].clock_core_max, (uint)((amdsmi_frequencies_core.frequency[f]+500000ull)/1000000ull));
		for(uint f=0u; f<amdsmi_frequencies_memory.num_supported; f++) gpus[g].clock_memory_max = max(gpus[g].clock_memory_max, 2u*(uint)((amdsmi_frequencies_memory.frequency[f]+500000ull)/1000000ull));
		gpus[g].memory_bus_width = amdsmi_vram_info.vram_bit_width;
		switch(amdsmi_vram_info.vram_type) { // memory type: 2 ((LP)DDR1-5, GDDR1-4, HBM1-4), 4 (GDDR5), 8 (GDDR5X, GDDR6), 16 (GDDR6X, GDDR6W, GDDR7)
			case AMDSMI_VRAM_TYPE_HBM   : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_HBM2  : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_HBM2E : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_HBM3  : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_HBM3E : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_DDR2  : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_DDR3  : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_DDR4  : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_DDR5  : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_GDDR1 : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_GDDR2 : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_GDDR3 : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_GDDR4 : gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_GDDR5 : gpus[g].memory_transfers_per_clock =  4u; break;
			case AMDSMI_VRAM_TYPE_GDDR6 : gpus[g].memory_transfers_per_clock =  8u; break;
			case AMDSMI_VRAM_TYPE_GDDR7 : gpus[g].memory_transfers_per_clock = 16u; break;
			case AMDSMI_VRAM_TYPE_LPDDR4: gpus[g].memory_transfers_per_clock =  2u; break;
			case AMDSMI_VRAM_TYPE_LPDDR5: gpus[g].memory_transfers_per_clock =  2u; break;
			default: gpus[g].memory_transfers_per_clock = 8u;
		}
		gpus[g].memory_bandwidth_max = gpus[g].clock_memory_max*gpus[g].memory_transfers_per_clock*gpus[g].memory_bus_width/8u; // amdsmi_vram_info.vram_max_bandwidth is broken
	}
}
void gpu_update_amd() {
	for(uint i=0u; i<(uint)amdsmi_devices.size(); i++) {
		const uint g = amdsmi_gpu_start+i;
		const amdsmi_processor_handle& amdsmi_device = amdsmi_devices[i];
		amdsmi_engine_usage_t amdsmi_engine_usage = {};
		amdsmi_vram_usage_t amdsmi_vram_usage = {};
		int64_t gpu_temperature_current=0ull, gpu_fan_current=0ull;
		amdsmi_power_info_t amdsmi_power_info = {};
		amdsmi_frequencies_t amdsmi_frequencies_core = {};
		amdsmi_frequencies_t amdsmi_frequencies_memory = {};
		amdsmi_pcie_info_t amdsmi_pcie_info = {};
		amdsmi_gpu_metrics_t amdsmi_gpu_metrics = {};
		uint64_t amdsmi_pcie_sent=0ull, amdsmi_pcie_received=0ull, amdsmi_pcie_max_pkt_sz=0ull;
		amdsmi_get_gpu_activity(amdsmi_device, &amdsmi_engine_usage);
		amdsmi_get_gpu_vram_usage(amdsmi_device, &amdsmi_vram_usage);
		amdsmi_get_temp_metric(amdsmi_device, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CURRENT, &gpu_temperature_current);
		amdsmi_get_gpu_fan_rpms(amdsmi_device, 0, &gpu_fan_current);
		amdsmi_get_power_info(amdsmi_device, &amdsmi_power_info);
		amdsmi_get_clk_freq(amdsmi_device, AMDSMI_CLK_TYPE_GFX, &amdsmi_frequencies_core); // in Hz
		amdsmi_get_clk_freq(amdsmi_device, AMDSMI_CLK_TYPE_MEM, &amdsmi_frequencies_memory); // in Hz
		const bool support_pcie_info = (AMDSMI_STATUS_SUCCESS==amdsmi_get_pcie_info(amdsmi_device, &amdsmi_pcie_info)); // broken, amdsmi_pcie_info.pcie_metric.pcie_bandwidth=4294967295
		const bool support_gpu_metrics = (AMDSMI_STATUS_SUCCESS==amdsmi_get_gpu_metrics_info(amdsmi_device, &amdsmi_gpu_metrics)); // broken, amdsmi_gpu_metrics.pcie_bandwidth_inst=18446744073709551615
		const bool support_gpu_pci = (AMDSMI_STATUS_SUCCESS==amdsmi_get_gpu_pci_throughput(amdsmi_device, &amdsmi_pcie_sent, &amdsmi_pcie_received, &amdsmi_pcie_max_pkt_sz)); // broken, returns 0's
		gpus[g].usage_current = amdsmi_engine_usage.gfx_activity; // in %
		gpus[g].memory_bandwidth_current = (gpus[g].memory_bandwidth_max*amdsmi_engine_usage.umc_activity+50u)/100u; // +50u for correct rounding
		gpus[g].memory_current = amdsmi_vram_usage.vram_used;
		gpus[g].memory_max = max(gpus[g].memory_max, amdsmi_vram_usage.vram_total); // harden against reading dropouts
		gpus[g].temperature_current = gpu_temperature_current>0ull ? (uint)gpu_temperature_current : gpus[g].temperature_current; // harden against reading dropouts
		gpus[g].temperature_max = gpus[g].temperature_current>0u ? 100u : 0u; // make 0 if no data is available
		gpus[g].fan_current = (uint)gpu_fan_current;
		gpus[g].power_current = (uint)amdsmi_power_info.socket_power; // in W
		gpus[g].power_max = amdsmi_power_info.power_limit>0u ? (amdsmi_power_info.power_limit+500000u)/1000000u : gpus[g].power_max; // harden against reading dropouts
		gpus[g].clock_core_current = (uint)((amdsmi_frequencies_core.frequency[amdsmi_frequencies_core.current]+500000ull)/1000000ull);
		gpus[g].clock_memory_current = 2u*(uint)((amdsmi_frequencies_memory.frequency[amdsmi_frequencies_memory.current]+500000ull)/1000000ull);
		gpus[g].pcie_bandwidth_current = support_gpu_pci ? (uint)((amdsmi_pcie_sent+amdsmi_pcie_received+500000ull)/1000000ull) : support_pcie_info&&amdsmi_pcie_info.pcie_metric.pcie_bandwidth<max_uint ? amdsmi_pcie_info.pcie_metric.pcie_bandwidth : support_gpu_metrics&&amdsmi_gpu_metrics.pcie_bandwidth_inst<max_ulong ? (uint)amdsmi_gpu_metrics.pcie_bandwidth_inst*1000u : max_uint; // harden against broken counters
		gpus[g].set_pcie_bandwidth_max(amdsmi_pcie_info.pcie_static.pcie_interface_version, (uint)amdsmi_pcie_info.pcie_metric.pcie_width); // harden against load-dependent reading // 2u*amdsmi_pcie_info.pcie_metric.pcie_speed // 123u*amdsmi_gpu_metrics.pcie_link_width*amdsmi_gpu_metrics.pcie_link_speed/5u
	}
}
void gpu_finalize_amd() {
	amdsmi_devices.clear();
	amdsmi_shut_down();
}
#endif // Linux
#endif // AMD_GPU

#ifdef INTEL_GPU
// (Windows) not available: -
// (Windows) broken: fan_max
// (Windows) unreliable/estimate: name
// (Linux) not available: -
// (Linux) broken: power_max, fan_current, fan_max
// (Linux) unreliable/estimate: clock_memory_current
#include "SYSMAN/include/zes_api.h" // https://github.com/oneapi-src/level-zero/blob/master/include/zes_api.h https://github.com/intel/xpumanager/blob/master/windows/winxpum/core/libs/ze_loader.lib
#pragma warning(disable:6385)
uint zes_gpu_start=0u, zes_gpu_number=0u;
vector<zes_device_handle_t> zes_devices;
vector<uint64_t> zes_last_active;
vector<uint64_t> zes_last_active_timestamp;
vector<uint64_t> zes_last_readwrite;
vector<uint64_t> zes_last_readwrite_timestamp;
vector<uint64_t> zes_last_energy;
vector<uint64_t> zes_last_energy_timestamp;
vector<uint64_t> zes_last_pci;
vector<uint64_t> zes_last_pci_timestamp;
vector<bool> zes_available_readwrite;
vector<bool> zes_available_energy;
vector<bool> zes_available_pci;
string zes_get_device_name(const uint device_id) {
	switch((int)device_id) { // https://github.com/intel/compute-runtime/blob/master/shared/source/dll/devices/devices_base.inl
		case 0x4909: return "Intel Iris Xe MAX 100";
		case 0x4905: return "Intel Iris Xe MAX";
		case 0x4907: return "Intel Server SG-18M";
		case 0x0BD9: return "Intel Data Center GPU Max 1100"; // Ponte Vecchio
		case 0x0BDA: return "Intel Data Center GPU Max 1100";
		case 0x0BDB: return "Intel Data Center GPU Max 1100";
		case 0x0B6E: return "Intel Data Center GPU Max 1100C";
		case 0x0BD7: return "Intel Data Center GPU Max 1350";
		case 0x0BD8: return "Intel Data Center GPU Max 1350";
		case 0x0B69: return "Intel Data Center GPU Max 1450";
		case 0x0BD5: return "Intel Data Center GPU Max 1550";
		case 0x0BD6: return "Intel Data Center GPU Max 1550";
		case 0x0BD4: return "Intel Data Center GPU Max 1550VG";
		case 0x56A6: return "Intel Arc A310"; // Alchemist dGPUs
		case 0x56A5: return "Intel Arc A380";
		case 0x56A2: return "Intel Arc A580";
		case 0x56A1: return "Intel Arc A750";
		case 0x56A0: return "Intel Arc A770";
		case 0x56B1: return "Intel Arc Pro A40/A50";
		case 0x56B3: return "Intel Arc Pro A60";
		case 0x56C1: return "Intel Data Center Flex 140";
		case 0x56C2: return "Intel Data Center Flex 170V";
		case 0x56C0: return "Intel Data Center Flex 170";
		case 0x56BB: return "Intel Arc A310E";
		case 0x56BC: return "Intel Arc A370E";
		case 0x56BA: return "Intel Arc A380E";
		case 0x56BD: return "Intel Arc A350E";
		case 0x56BF: return "Intel Arc A580E";
		case 0x56BE: return "Intel Arc A750E";
		case 0x56AF: return "Intel Arc A760A";
		case 0x5694: return "Intel Arc A350M"; // Alchemist Mobile dGPUs
		case 0x5693: return "Intel Arc A370M";
		case 0x5697: return "Intel Arc A530M";
		case 0x5692: return "Intel Arc A550M";
		case 0x5696: return "Intel Arc A570M";
		case 0x5691: return "Intel Arc A730M";
		case 0x5690: return "Intel Arc A770M";
		case 0x56B0: return "Intel Arc Pro A30M";
		case 0x56B2: return "Intel Arc Pro A60M";
		case 0xE20C: return "Intel Arc B570"; // Battlemage dGPUs
		case 0xE209: return "Intel Arc B580";
		case 0xE20B: return "Intel Arc B580";
		case 0xE212: return "Intel Arc Pro B50";
		case 0xE211: return "Intel Arc Pro B60";
		case 0xE222: return "Intel Arc Pro B65";
		case 0xE223: return "Intel Arc Pro B70";
		case 0x46D3: return "Intel Graphics";
		case 0x46D4: return "Intel Graphics";
		case 0xA7AA: return "Intel Graphics"; // Core 7 250H
		case 0xA7AB: return "Intel Graphics"; // Core 7 270H
		case 0xA7AC: return "Intel Graphics"; // Core 7 250U
		case 0xA7AD: return "Intel Graphics";
		case 0x462A: return "Intel UHD Graphics";
		case 0x4626: return "Intel UHD Graphics";
		case 0x4628: return "Intel UHD Graphics";
		case 0x4688: return "Intel UHD Graphics"; // Core i9-12950HX
		case 0x468A: return "Intel UHD Graphics";
		case 0x468B: return "Intel UHD Graphics";
		case 0x46A3: return "Intel UHD Graphics";
		case 0x46B3: return "Intel UHD Graphics";
		case 0x46C3: return "Intel UHD Graphics";
		case 0x46D0: return "Intel UHD Graphics"; // Core i3-N305
		case 0x46D1: return "Intel UHD Graphics"; // N100
		case 0x46D2: return "Intel UHD Graphics";
		case 0x9A60: return "Intel UHD Graphics"; // Core i9-11980HK
		case 0x9A68: return "Intel UHD Graphics";
		case 0x9A70: return "Intel UHD Graphics";
		case 0x9A78: return "Intel UHD Graphics";
		case 0xA788: return "Intel UHD Graphics";
		case 0xA78A: return "Intel UHD Graphics";
		case 0xA78B: return "Intel UHD Graphics";
		case 0xA720: return "Intel UHD Graphics";
		case 0xA721: return "Intel UHD Graphics";
		case 0xA7A8: return "Intel UHD Graphics";
		case 0xA7A9: return "Intel UHD Graphics";
		case 0x4693: return "Intel UHD 710"; // Pentium Gold G7400
		case 0x4682: return "Intel UHD 730"; // Core i5-13400
		case 0x4692: return "Intel UHD 730"; // Core i5-12400T
		case 0xA782: return "Intel UHD 730"; // Core i5-13400
		case 0x4C8B: return "Intel UHD 730"; // Core i5-11400
		case 0x4C8A: return "Intel UHD 750"; // Core i9-11900K
		case 0x4C90: return "Intel UHD P750";
		case 0x4C9A: return "Intel UHD P750"; // Xeon E-2388G
		case 0x4680: return "Intel UHD 770"; // Core i9-12900KS
		case 0x4690: return "Intel UHD 770";
		case 0xA780: return "Intel UHD 770"; // Core i9-14900KS
		case 0x46A6: return "Intel Iris Xe"; // Core i9-12900H
		case 0x46A8: return "Intel Iris Xe"; // Core i7-1265U
		case 0x46AA: return "Intel Iris Xe"; // Core i7-1260U
		case 0x9A40: return "Intel Iris Xe"; // Core i7-1180G7
		case 0x9A49: return "Intel Iris Xe"; // Core i7-1195G7
		case 0xA7A0: return "Intel Iris Xe";
		case 0xA7A1: return "Intel Iris Xe"; // Core i7-1365U
		case 0x4908: return "Intel Iris Xe";
		case 0x7D40: return "Intel Graphics";
		case 0x7D41: return "Intel Graphics"; // Core Ultra 7 265U
		case 0x7D45: return "Intel Graphics"; // Core Ultra 7 165UL
		case 0x7D67: return "Intel Graphics"; // Core Ultra 9 285K
		case 0x7DD1: return "Intel Graphics";
		case 0x7DD5: return "Intel Graphics";
		case 0x7D55: return "Intel Arc Graphics"; // Core Ultra 9 185H
		case 0x7D51: return "Intel Arc 130T/140T"; // Core Ultra 9 285H
		case 0x6420: return "Intel Graphics";
		case 0x64B0: return "Intel Graphics";
		case 0x64A0: return "Intel Arc 130V/140V"; // Core Ultra 9 288V
		case 0xB08F: return "Intel Graphics";
		case 0xB090: return "Intel Graphics"; // Core Ultra 7 365
		case 0xB0A0: return "Intel Graphics"; // Core Ultra 9 386H
		case 0xB081: return "Intel Arc B370"; // Core Ultra 5 338H
		case 0xB083: return "Intel Arc B370"; // Core Ultra 5 338H
		case 0xB085: return "Intel Arc B370"; // Core Ultra 5 338H
		case 0xB087: return "Intel Arc B370"; // Core Ultra 5 338H
		case 0xB080: return "Intel Arc B390"; // Core Ultra X9 388H
		case 0xB082: return "Intel Arc B390"; // Core Ultra X9 388H
		case 0xB084: return "Intel Arc B390"; // Core Ultra X9 388H
		case 0xB086: return "Intel Arc B390"; // Core Ultra X9 388H
		case 0x4906: return "Intel DG1"; // generic device names
		case 0x4F80: return "Intel Arc Alchemist";
		case 0x4F81: return "Intel Arc Alchemist";
		case 0x4F82: return "Intel Arc Alchemist";
		case 0x4F83: return "Intel Arc Alchemist";
		case 0x4F84: return "Intel Arc Alchemist";
		case 0x4F85: return "Intel Arc Alchemist";
		case 0x4F86: return "Intel Arc Alchemist";
		case 0x4F87: return "Intel Arc Alchemist";
		case 0x4F88: return "Intel Arc Alchemist";
		case 0x5695: return "Intel Arc Alchemist";
		case 0x56A3: return "Intel Arc Alchemist";
		case 0x56A4: return "Intel Arc Alchemist";
		case 0xE202: return "Intel Arc Battlemage";
		case 0xE20D: return "Intel Arc Battlemage";
		case 0xE210: return "Intel Arc Battlemage";
		case 0xE215: return "Intel Arc Battlemage";
		case 0xE216: return "Intel Arc Battlemage";
		case 0xE220: return "Intel Arc Battlemage";
		case 0xE221: return "Intel Arc Battlemage";
		case 0x0BD0: return "Intel Ponte Vecchio";
		case 0x674C: return "Intel Crescent Island";
		case 0x9A59: return "Intel TgllpHw1x6x16";
		case 0x4C80: return "Intel Rocket Lake";
		case 0x4C8C: return "Intel Rocket Lake";
		case 0xA781: return "Intel Alder Lake S";
		case 0xA783: return "Intel Alder Lake S";
		case 0xA789: return "Intel Alder Lake S";
		case 0x46A0: return "Intel Alder Lake P";
		case 0x46B0: return "Intel Alder Lake P";
		case 0x46A1: return "Intel Alder Lake P";
		case 0x46B1: return "Intel Alder Lake P";
		case 0x46C0: return "Intel Alder Lake P";
		case 0x46C1: return "Intel Alder Lake P";
		case 0xB0B0: return "Intel Panther Lake";
		case 0xFD80: return "Intel Panther Lake";
		case 0xFD81: return "Intel Panther Lake";
		case 0xD740: return "Intel Nova Lake S";
		case 0xD741: return "Intel Nova Lake S";
		case 0xD742: return "Intel Nova Lake S";
		case 0xD743: return "Intel Nova Lake S";
		case 0xD744: return "Intel Nova Lake S";
		case 0xD745: return "Intel Nova Lake S";
	}
	return "Intel GPU ["+to_string_hex((ushort)device_id)+"]";
}
uint zes_get_memory_bandwidth_max(const uint device_id) {
	switch((int)device_id) { // https://github.com/intel/compute-runtime/blob/master/shared/source/dll/devices/devices_base.inl
		case 0x4909: return   34128u; // Intel Iris Xe MAX 100
		case 0x4905: return   68000u; // Intel Iris Xe MAX
		case 0x0BD9: return 1228800u; // Intel Data Center GPU Max 1100 // Ponte Vecchio
		case 0x0BDA: return 1228800u; // Intel Data Center GPU Max 1100
		case 0x0BDB: return 1228800u; // Intel Data Center GPU Max 1100
		case 0x0B6E: return 1228800u; // Intel Data Center GPU Max 1100C
		case 0x0BD5: return 3276800u; // Intel Data Center GPU Max 1550
		case 0x0BD6: return 3276800u; // Intel Data Center GPU Max 1550
		case 0x0BD4: return 3276800u; // Intel Data Center GPU Max 1550VG
		case 0x56A6: return  124000u; // Intel Arc A310 // Alchemist dGPUs
		case 0x56A5: return  186000u; // Intel Arc A380
		case 0x56A2: return  512000u; // Intel Arc A580
		case 0x56A1: return  512000u; // Intel Arc A750
		case 0x56A0: return  560000u; // Intel Arc A770
		case 0x56B1: return  192000u; // Intel Arc Pro A40/A50
		case 0x56B3: return  384000u; // Intel Arc Pro A60
		case 0x56C1: return  336000u; // Intel Data Center Flex 140
		case 0x56C2: return  576000u; // Intel Data Center Flex 170V
		case 0x56C0: return  576000u; // Intel Data Center Flex 170
		case 0x56BB: return  124000u; // Intel Arc A310E
		case 0x56BD: return  112000u; // Intel Arc A350E
		case 0x56BC: return  112000u; // Intel Arc A370E
		case 0x56BA: return  186000u; // Intel Arc A380E
		case 0x56BF: return  560000u; // Intel Arc A580E
		case 0x56BE: return  560000u; // Intel Arc A750E
		case 0x56AF: return  512000u; // Intel Arc A760A
		case 0x5694: return  112000u; // Intel Arc A350M // Alchemist Mobile dGPUs
		case 0x5693: return  112000u; // Intel Arc A370M
		case 0x5697: return  224000u; // Intel Arc A530M
		case 0x5692: return  224000u; // Intel Arc A550M
		case 0x5696: return  256000u; // Intel Arc A570M
		case 0x5691: return  336000u; // Intel Arc A730M
		case 0x5690: return  512000u; // Intel Arc A770M
		case 0x56B0: return  112000u; // Intel Arc Pro A30M
		case 0x56B2: return  256000u; // Intel Arc Pro A60M
		case 0xE20C: return  380000u; // Intel Arc B570 // Battlemage dGPUs
		case 0xE209: return  456000u; // Intel Arc B580
		case 0xE20B: return  456000u; // Intel Arc B580
		case 0xE212: return  224000u; // Intel Arc Pro B50
		case 0xE211: return  456000u; // Intel Arc Pro B60
		case 0xE222: return  608000u; // Intel Arc Pro B65
		case 0xE223: return  608000u; // Intel Arc Pro B70
		case 0xA7AA: return  102400u; // Intel Graphics // Core 7 250H
		case 0xA7AB: return  102400u; // Intel Graphics // Core 7 270H
		case 0xA7AC: return  102400u; // Intel Graphics // Core 7 250U
		case 0x4688: return   76800u; // i9-12950HX
		case 0x46D0: return   38400u; // Intel UHD Graphics // i3-N305
		case 0x46D1: return   38400u; // Intel UHD Graphics // N100
		case 0x9A60: return   51200u; // Intel UHD Graphics // i9-11980HK
		case 0x4693: return   76800u; // Intel UHD 710 // Pentium Gold G7400
		case 0x4682: return   76800u; // Intel UHD 730 // Core i5-13400
		case 0x4692: return   76800u; // Intel UHD 730 // Core i5-12400T
		case 0xA782: return   76800u; // Intel UHD 730 // Core i5-13400
		case 0x4C8B: return   51200u; // Intel UHD 730 // Core i5-11400
		case 0x4C8A: return   51200u; // Intel UHD 750 // Core i9-11900K
		case 0x4C9A: return   51200u; // Intel UHD P750 // Xeon E-2388G
		case 0x4680: return   76800u; // Intel UHD 770 // Core i9-12900KS
		case 0xA780: return   89600u; // Intel UHD 770 // Core i9-14900KS
		case 0x46A6: return   83200u; // Intel Iris Xe // Core i9-12900H
		case 0x46A8: return   83200u; // Intel Iris Xe // Core i7-1265U
		case 0x46AA: return   83200u; // Intel Iris Xe // Core i7-1260U
		case 0x9A40: return   68272u; // Intel Iris Xe // Core i7-1180G7
		case 0x9A49: return   68272u; // Intel Iris Xe // Core i7-1195G7
		case 0xA7A1: return  102400u; // Intel Iris Xe // Core i7-1365U
		case 0x7D41: return  134400u; // Intel Graphics // Core Ultra 7 265U
		case 0x7D45: return   89600u; // Intel Graphics // Core Ultra 7 165UL
		case 0x7D67: return  102400u; // Intel Graphics // Core Ultra 9 285K
		case 0x7D55: return  119472u; // Intel Arc Graphics // Core Ultra 9 185H
		case 0x7D51: return  134400u; // Intel Arc 130T/140T // Core Ultra 9 285H
		case 0x64A0: return  136528u; // Intel Arc 130V/140V // Core Ultra 9 288V
		case 0xB090: return  119472u; // Intel Graphics // Core Ultra 7 365
		case 0xB0A0: return  136528u; // Intel Graphics // Core Ultra 9 386H
		case 0xB081: return  136528u; // Intel Arc B370 // Intel Core Ultra 5 338H
		case 0xB083: return  136528u; // Intel Arc B370 // Intel Core Ultra 5 338H
		case 0xB085: return  136528u; // Intel Arc B370 // Intel Core Ultra 5 338H
		case 0xB087: return  136528u; // Intel Arc B370 // Intel Core Ultra 5 338H
		case 0xB080: return  153600u; // Intel Arc B390 // Intel Core Ultra X9 388H
		case 0xB082: return  153600u; // Intel Arc B390 // Intel Core Ultra X9 388H
		case 0xB084: return  153600u; // Intel Arc B390 // Intel Core Ultra X9 388H
		case 0xB086: return  153600u; // Intel Arc B390 // Intel Core Ultra X9 388H
	}
	return 0u;
}
bool zes_get_engine_usage(const uint g, const uint zes_engine_count, const zes_engine_handle_t* zes_engine_handles, const zes_engine_group_t& zes_engine_group) {
	for(uint j=0u; j<zes_engine_count; j++) {
		zes_engine_properties_t zes_engine_properties = {};
		zesEngineGetProperties(zes_engine_handles[j], &zes_engine_properties);
		if(zes_engine_properties.type==zes_engine_group) {
			zes_engine_stats_t zes_engine_stats = {};
			zesEngineGetActivity(zes_engine_handles[j], &zes_engine_stats);
			const ulong zes_usage_interval = zes_engine_stats.timestamp-zes_last_active_timestamp[g-zes_gpu_start];
			const uint gpu_usage_current = zes_usage_interval>0ull ? (uint)((100ull*(zes_engine_stats.activeTime-zes_last_active[g-zes_gpu_start])+zes_usage_interval/2ull)/zes_usage_interval) : gpus[g].usage_current; // reuse last value if interval is 0
			gpus[g].usage_current = gpu_usage_current<2u*gpus[g].usage_max ? min(gpu_usage_current, gpus[g].usage_max) : 0u; // harden against spikes
			zes_engine_stats.timestamp;
			zes_last_active[g-zes_gpu_start] = zes_engine_stats.activeTime;
			zes_last_active_timestamp[g-zes_gpu_start] = zes_engine_stats.timestamp;
			return true;
		}
	}
	return false;
}
void gpu_initialize_intel() {
#ifdef _WIN32 // return if ze_loader.dll is not available
	if(!LoadLibrary("ze_loader.dll")) return;
#endif // Windows
	if(zesInit(0)!=ZE_RESULT_SUCCESS) return;
	uint zes_driver_handle_count = 0u;
	zesDriverGet(&zes_driver_handle_count, nullptr);
	vector<zes_driver_handle_t> zes_driver_handles(zes_driver_handle_count);
	zesDriverGet(&zes_driver_handle_count, &zes_driver_handles[0]);
	int igpu_at_index = -1;
	for(const zes_driver_handle_t& zes_driver_handle : zes_driver_handles) {
		uint zes_device_count = 0;
		zesDeviceGet(zes_driver_handle, &zes_device_count, nullptr);
		vector<zes_device_handle_t> zes_devices_to_add(zes_device_count);
		zesDeviceGet(zes_driver_handle, &zes_device_count, &zes_devices_to_add[0]);
		for(const zes_device_handle_t& zes_device_to_add : zes_devices_to_add) {
			zes_devices.push_back(zes_device_to_add);
			zes_device_properties_t zes_device_properties = {};
			zesDeviceGetProperties(zes_device_to_add, &zes_device_properties);
			if((zes_device_properties.core.flags&ZE_DEVICE_PROPERTY_FLAG_INTEGRATED)&&igpu_at_index==-1) igpu_at_index = (int)zes_devices.size()-1;
		}
	}
	if(igpu_at_index>=0) { // move iGPU to be last
		const zes_device_handle_t zes_device_handle_igpu = zes_devices[igpu_at_index];
		zes_devices.erase(zes_devices.begin()+igpu_at_index);
		zes_devices.push_back(zes_device_handle_igpu);
	}
	zes_last_active.resize(zes_devices.size());
	zes_last_active_timestamp.resize(zes_devices.size());
	zes_last_readwrite.resize(zes_devices.size());
	zes_last_readwrite_timestamp.resize(zes_devices.size());
	zes_last_energy.resize(zes_devices.size());
	zes_last_energy_timestamp.resize(zes_devices.size());
	zes_last_pci.resize(zes_devices.size());
	zes_last_pci_timestamp.resize(zes_devices.size());
	zes_available_readwrite.resize(zes_devices.size());
	zes_available_energy.resize(zes_devices.size());
	zes_available_pci.resize(zes_devices.size());
	zes_gpu_start = gpu_number;
	zes_gpu_number = (uint)zes_devices.size();
	gpu_number += zes_gpu_number;
	gpus.resize(gpu_number);
	for(uint i=0u; i<(uint)zes_devices.size(); i++) {
		const uint g = zes_gpu_start+i;
		const zes_device_handle_t& zes_device = zes_devices[i];
		zes_device_properties_t zes_device_properties = {};
		zesDeviceGetProperties(zes_device, &zes_device_properties);
		const string zes_gpu_name = trim(string(zes_device_properties.core.name));
		gpus[g].name = length(zes_gpu_name)>0u&&!contains(zes_gpu_name, "[0x") ? clean_device_name(zes_gpu_name) : zes_get_device_name(zes_device_properties.core.deviceId); // harden against broken counters
		gpus[g].vendor = 'I';
		gpus[g].fan_max = 5000u; // no data available
		uint zes_mem_count = 0u;
		zesDeviceEnumMemoryModules(zes_device, &zes_mem_count, nullptr);
		zes_mem_handle_t* zes_mem_handles = new zes_mem_handle_t[zes_mem_count];
		zesDeviceEnumMemoryModules(zes_device, &zes_mem_count, zes_mem_handles);
		for(uint j=0u; j<min(zes_mem_count, 1u); j++) {
			zes_mem_bandwidth_t zes_mem_bandwidth = {};
			zes_mem_properties_t zes_mem_properties = {};
			zes_mem_state_t zes_mem_state = {};
			zesMemoryGetBandwidth(zes_mem_handles[j], &zes_mem_bandwidth);
			zesMemoryGetProperties(zes_mem_handles[j], &zes_mem_properties);
			zesMemoryGetState(zes_mem_handles[j], &zes_mem_state);
			uint zes_memory_bandwidth_max = (uint)((zes_mem_bandwidth.maxBandwidth+500000ull)/1000000ull); // zes_mem_bandwidth.maxBandwidth may wrongly report bandwidth in bits/s instead of Bytes/s, so divide by 8
			gpus[g].memory_bandwidth_max = zes_memory_bandwidth_max>0u ? (zes_memory_bandwidth_max<3300000u ? zes_memory_bandwidth_max : zes_memory_bandwidth_max/8u) : zes_get_memory_bandwidth_max(zes_device_properties.core.deviceId); // harden against broken counters
			gpus[g].memory_max = (uint)((max(zes_mem_properties.physicalSize, zes_mem_state.size)+524288ull)/1048576ull); // harden against broken counters
			const uint memory_max_gb = (gpus[g].memory_max+500u)/1000u;
			uint memory_bus_width_fallback = memory_max_gb*16u; // harden against broken counters
			switch(memory_max_gb) {
				case  4u: memory_bus_width_fallback =  64u; break; // Intel Arc A310
				case  6u: memory_bus_width_fallback =  96u; break; // Intel Arc A380
				case  8u: memory_bus_width_fallback = 256u; break; // Intel Arc A580/A750/A770
				case 10u: memory_bus_width_fallback = 160u; break; // Intel Arc B570
				case 12u: memory_bus_width_fallback = 192u; break; // Intel Arc B580
				case 16u: memory_bus_width_fallback = 256u; break; // Intel Arc A770
				case 24u: memory_bus_width_fallback = 192u; break; // Intel Arc Pro B60
				case 32u: memory_bus_width_fallback = 256u; break; // Intel Arc Pro B65/B70
			}
			if(zes_device_properties.core.flags&ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) memory_bus_width_fallback = 128u; // dual-channel (128-bit) memory for iGPUs
			gpus[g].memory_bus_width = zes_mem_properties.busWidth>0 ? (uint)(zes_mem_properties.busWidth>256 ? zes_mem_properties.busWidth/2 : zes_mem_properties.busWidth) : memory_bus_width_fallback; // zes_mem_properties.busWidth may wrongly report 2x bus width
			switch(zes_mem_properties.type) { // memory type: 2 ((LP)DDR1-5, GDDR1-4, HBM1-4), 4 (GDDR5), 8 (GDDR5X, GDDR6), 16 (GDDR6X, GDDR6W, GDDR7)
				case ZES_MEM_TYPE_HBM	: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_DDR	: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_DDR3	: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_DDR4	: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_DDR5	: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_LPDDR : gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_LPDDR3: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_LPDDR4: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_LPDDR5: gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_GDDR4 : gpus[g].memory_transfers_per_clock =  2u; break;
				case ZES_MEM_TYPE_GDDR5 : gpus[g].memory_transfers_per_clock =  4u; break;
				case ZES_MEM_TYPE_GDDR5X: gpus[g].memory_transfers_per_clock =  8u; break;
				case ZES_MEM_TYPE_GDDR6 : gpus[g].memory_transfers_per_clock =  8u; break;
				case ZES_MEM_TYPE_GDDR6X: gpus[g].memory_transfers_per_clock = 16u; break;
				case ZES_MEM_TYPE_GDDR7 : gpus[g].memory_transfers_per_clock = 16u; break;
				default: gpus[g].memory_transfers_per_clock = (zes_device_properties.core.flags&ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) ? 2u : 8u;
			}
		}
		delete[] zes_mem_handles;
		uint zes_freq_count = 0u;
		zesDeviceEnumFrequencyDomains(zes_device, &zes_freq_count, nullptr);
		zes_freq_handle_t* zes_freq_handles = new zes_freq_handle_t[zes_freq_count];
		zesDeviceEnumFrequencyDomains(zes_device, &zes_freq_count, zes_freq_handles);
		for(uint j=0u; j<zes_freq_count; j++) {
			zes_freq_properties_t zes_freq_properties = {};
			zesFrequencyGetProperties(zes_freq_handles[j], &zes_freq_properties);
			if(zes_freq_properties.type==ZES_FREQ_DOMAIN_GPU) {
				zes_freq_range_t zes_freq_range = {};
				zesFrequencyGetRange(zes_freq_handles[j], &zes_freq_range);
				gpus[g].clock_core_max = to_uint(fmax(zes_freq_properties.max, zes_freq_range.max)); // harden against broken counters
			}
			if(zes_freq_properties.type==ZES_FREQ_DOMAIN_MEMORY) {
				zes_freq_range_t zes_freq_range = {};
				zesFrequencyGetRange(zes_freq_handles[j], &zes_freq_range);
				gpus[g].clock_memory_max = to_uint(fmax(zes_freq_properties.max, zes_freq_range.max)); // harden against broken counters
			}
		}
		delete[] zes_freq_handles;
		if(gpus[g].clock_core_max==0u) gpus[g].clock_core_max = zes_device_properties.core.coreClockRate; // harden against broken counters
		if(gpus[g].clock_memory_max==0u) gpus[g].clock_memory_max = gpus[g].memory_bus_width>0u ? gpus[g].memory_bandwidth_max*8u/(gpus[g].memory_transfers_per_clock*gpus[g].memory_bus_width) : 0u; // harden against broken counters
	}
}
void gpu_update_intel() {
	for(uint i=0u; i<(uint)zes_devices.size(); i++) {
		const uint g = zes_gpu_start+i;
		const zes_device_handle_t& zes_device = zes_devices[i];
		uint zes_engine_count = 0u;
		zesDeviceEnumEngineGroups(zes_device, &zes_engine_count, nullptr);
		zes_engine_handle_t* zes_engine_handles = new zes_engine_handle_t[zes_engine_count];
		zesDeviceEnumEngineGroups(zes_device, &zes_engine_count, zes_engine_handles);
		bool zes_engine_found = false; // check engine availability in this order: ZES_ENGINE_GROUP_COMPUTE_ALL, ZES_ENGINE_GROUP_COMPUTE_SINGLE, ZES_ENGINE_GROUP_RENDER_ALL, ZES_ENGINE_GROUP_RENDER_SINGLE, ZES_ENGINE_GROUP_ALL
		if(!zes_engine_found) zes_engine_found = zes_engine_found||zes_get_engine_usage(g, zes_engine_count, zes_engine_handles, ZES_ENGINE_GROUP_COMPUTE_ALL   ); // harden against unavailable counters
		if(!zes_engine_found) zes_engine_found = zes_engine_found||zes_get_engine_usage(g, zes_engine_count, zes_engine_handles, ZES_ENGINE_GROUP_COMPUTE_SINGLE);
		if(!zes_engine_found) zes_engine_found = zes_engine_found||zes_get_engine_usage(g, zes_engine_count, zes_engine_handles, ZES_ENGINE_GROUP_RENDER_ALL    );
		if(!zes_engine_found) zes_engine_found = zes_engine_found||zes_get_engine_usage(g, zes_engine_count, zes_engine_handles, ZES_ENGINE_GROUP_RENDER_SINGLE );
		if(!zes_engine_found) zes_engine_found = zes_engine_found||zes_get_engine_usage(g, zes_engine_count, zes_engine_handles, ZES_ENGINE_GROUP_ALL           );
		delete[] zes_engine_handles;
		if(!zes_engine_found) gpus[g].usage_current = gpus[g].usage_max = max_uint; // no data available
		uint zes_mem_count = 0u;
		zesDeviceEnumMemoryModules(zes_device, &zes_mem_count, nullptr);
		zes_mem_handle_t* zes_mem_handles = new zes_mem_handle_t[zes_mem_count];
		zesDeviceEnumMemoryModules(zes_device, &zes_mem_count, zes_mem_handles);
		for(uint j=0u; j<min(zes_mem_count, 1u); j++) {
			zes_mem_bandwidth_t zes_mem_bandwidth = {};
			zes_mem_properties_t zes_mem_properties = {};
			zes_mem_state_t zes_mem_state = {};
			zesMemoryGetBandwidth(zes_mem_handles[j], &zes_mem_bandwidth);
			zesMemoryGetProperties(zes_mem_handles[j], &zes_mem_properties);
			zesMemoryGetState(zes_mem_handles[j], &zes_mem_state);
			const ulong zes_memory_bandwidth_interval = zes_mem_bandwidth.timestamp-zes_last_readwrite_timestamp[i];
			const uint gpu_memory_bandwidth_current = zes_memory_bandwidth_interval>0ull ? (uint)((zes_mem_bandwidth.readCounter+zes_mem_bandwidth.writeCounter-zes_last_readwrite[i]+zes_memory_bandwidth_interval/2ull)/zes_memory_bandwidth_interval) : gpus[g].memory_bandwidth_current; // reuse last value if interval is 0
			zes_available_readwrite[i] = zes_available_readwrite[i]||zes_mem_bandwidth.readCounter+zes_mem_bandwidth.writeCounter>0ull; // harden against reading dropouts
			gpus[g].memory_bandwidth_current = zes_available_readwrite[i] ? (gpu_memory_bandwidth_current<2u*gpus[g].memory_bandwidth_max ? gpu_memory_bandwidth_current : 0u) : max_uint; // harden against spikes
			uint zes_memory_bandwidth_max = (uint)((zes_mem_bandwidth.maxBandwidth+500000ull)/1000000ull); // harden against reading dropouts
			gpus[g].memory_bandwidth_max = max(gpus[g].memory_bandwidth_max, zes_memory_bandwidth_max<3300000u ? zes_memory_bandwidth_max : zes_memory_bandwidth_max/8u); // zes_mem_bandwidth.maxBandwidth may wrongly report bandwidth in bits/s instead of Bytes/s, so divide by 8
			zes_last_readwrite[i] = zes_mem_bandwidth.readCounter+zes_mem_bandwidth.writeCounter;
			zes_last_readwrite_timestamp[i] = zes_mem_bandwidth.timestamp;
			gpus[g].memory_max = max(gpus[g].memory_max, (uint)((max(zes_mem_properties.physicalSize, zes_mem_state.size)+524288ull)/1048576ull)); // harden against broken counters and reading dropouts
			gpus[g].memory_current = zes_mem_state.size>0ull ? gpus[g].memory_max-min((uint)((zes_mem_state.free+524288ull)/1048576ull), gpus[g].memory_max) : 0u; // harden against reading dropouts
		}
		delete[] zes_mem_handles;
		if(zes_mem_count==0u) gpus[g].memory_bandwidth_current = gpus[g].memory_max = max_uint;
		uint zes_temp_count = 0u;
		zesDeviceEnumTemperatureSensors(zes_device, &zes_temp_count, nullptr);
		zes_temp_handle_t* zes_temp_handles = new zes_temp_handle_t[zes_temp_count];
		zesDeviceEnumTemperatureSensors(zes_device, &zes_temp_count, zes_temp_handles);
		bool zes_temp_sensors_gpu_found = false;
		for(uint j=0u; j<zes_temp_count; j++) {
			zes_temp_properties_t zes_temp_properties = {};
			zesTemperatureGetProperties(zes_temp_handles[j], &zes_temp_properties);
			if(zes_temp_properties.type==ZES_TEMP_SENSORS_GPU) {
				double gpu_temperature = 0.0;
				zesTemperatureGetState(zes_temp_handles[j], &gpu_temperature);
				gpus[g].temperature_current = gpu_temperature>0.0 ? to_uint(gpu_temperature) : gpus[g].temperature_current; // harden against reading dropouts
				zes_temp_sensors_gpu_found = true;
				break;
			}
		}
		if(!zes_temp_sensors_gpu_found) { // harden against unavailable counters
			double gpu_temperature = 0.0;
			for(uint j=0u; j<zes_temp_count; j++) {
				double gpu_temperature_j = 0.0;
				zesTemperatureGetState(zes_temp_handles[j], &gpu_temperature_j);
				gpu_temperature = fmax(gpu_temperature, gpu_temperature_j);
			}
			gpus[g].temperature_current = gpu_temperature>0.0 ? to_uint(gpu_temperature) : gpus[g].temperature_current; // harden against reading dropouts
		}
		delete[] zes_temp_handles;
		if(zes_temp_count==0u||gpus[g].temperature_current==0u) gpus[g].temperature_max = max_uint; // no data available
		zes_pwr_handle_t zes_pwr_handle = {};
		bool zes_pwr_handle_found = true;
		if(zesDeviceGetCardPowerDomain(zes_device, &zes_pwr_handle)!=ZE_RESULT_SUCCESS) { // harden against broken counters
			uint zes_pwr_count = 0u;
			zesDeviceEnumPowerDomains(zes_device, &zes_pwr_count, nullptr);
			zes_pwr_handle_t* zes_pwr_handles = new zes_pwr_handle_t[zes_pwr_count];
			zesDeviceEnumPowerDomains(zes_device, &zes_pwr_count, zes_pwr_handles);
			if(zes_pwr_count>0u) zes_pwr_handle = zes_pwr_handles[0]; // https://oneapi-src.github.io/level-zero-spec/level-zero/latest/sysman/PROG.html#device-component-management
			else zes_pwr_handle_found = false;
			delete[] zes_pwr_handles;
		}
		if(zes_pwr_handle_found) {
			zes_power_energy_counter_t zes_power_energy_counter = {};
			zesPowerGetEnergyCounter(zes_pwr_handle, &zes_power_energy_counter);
			const ulong zes_power_interval = zes_power_energy_counter.timestamp-zes_last_energy_timestamp[i];
			const uint gpu_power_current = zes_power_interval>0ull ? (uint)((zes_power_energy_counter.energy-zes_last_energy[i]+zes_power_interval/2ull)/zes_power_interval) : gpus[g].power_current; // reuse last value if interval is 0
			zes_available_energy[i] = zes_available_energy[i]||zes_power_energy_counter.energy>0ull; // harden against reading dropouts
			gpus[g].power_current = zes_available_energy[i] ? (gpu_power_current<2u*gpus[g].power_max ? gpu_power_current : 0u) : max_uint; // harden against spikes
			zes_last_energy[i] = zes_power_energy_counter.energy;
			zes_last_energy_timestamp[i] = zes_power_energy_counter.timestamp;
			zes_power_properties_t zes_power_properties = {};
			zes_power_sustained_limit_t zes_power_sustained_limit = {};
			zes_power_burst_limit_t zes_power_burst_limit = {};
			zes_power_peak_limit_t zes_power_peak_limit = {};
			zesPowerGetProperties(zes_pwr_handle, &zes_power_properties);
			zesPowerGetLimits(zes_pwr_handle, &zes_power_sustained_limit, &zes_power_burst_limit, &zes_power_peak_limit); // returns ZE_RESULT_ERROR_UNSUPPORTED_FEATURE
			const uint gpu_power_max = ((uint)max(zes_power_properties.defaultLimit, zes_power_sustained_limit.power)+500u)/1000u; // harden against broken counters
			if(gpu_power_max>0u) { // harden against reading dropouts
				gpus[g].power_max = gpu_power_max;
			} else { // harden against broken counters
				uint power_limit_ext_counter = 0u;
				zesPowerGetLimitsExt(zes_pwr_handle, &power_limit_ext_counter, nullptr);
				zes_power_limit_ext_desc_t* zes_power_limit_ext_descs = new zes_power_limit_ext_desc_t[power_limit_ext_counter];
				zesPowerGetLimitsExt(zes_pwr_handle, &power_limit_ext_counter, zes_power_limit_ext_descs);
				bool zes_power_limit_sustained_found = false;
				for(uint j=0u; j<power_limit_ext_counter; j++) {
					if(zes_power_limit_ext_descs[j].level==ZES_POWER_LEVEL_BURST) { // only ZES_POWER_LEVEL_BURST returns correct TDP
						gpus[g].power_max = zes_power_limit_ext_descs[j].limit>0 ? ((uint)zes_power_limit_ext_descs[j].limit+500u)/1000u : gpus[g].power_max; // harden against reading dropouts
						zes_power_limit_sustained_found = true;
					}
				}
				if(!zes_power_limit_sustained_found) { // harden against broken counters
					int zes_power_limit_mW = max_int;
					for(uint j=0u; j<power_limit_ext_counter; j++) {
						if(zes_power_limit_ext_descs[j].limit>0) zes_power_limit_mW = min(zes_power_limit_mW, zes_power_limit_ext_descs[j].limit); // select smallest non-zero power limit
					}
					gpus[g].power_max = zes_power_limit_mW>0&&zes_power_limit_mW<max_int ? ((uint)zes_power_limit_mW+500u)/1000u : gpus[g].power_max;
				}
				delete[] zes_power_limit_ext_descs;
			}
		} else {
			gpus[g].power_current = gpus[g].power_max = max_uint; // no data available
		}
		uint zes_fan_count = 0u;
		zesDeviceEnumFans(zes_device, &zes_fan_count, nullptr);
		zes_fan_handle_t* zes_fan_handles = new zes_fan_handle_t[zes_fan_count];
		zesDeviceEnumFans(zes_device, &zes_fan_count, zes_fan_handles);
		uint gpu_fan_rpm = 0u;
		for(uint j=0u; j<zes_fan_count; j++) {
			int gpu_fan = 0;
			zes_fan_config_t zes_fan_config = {};
			zes_fan_properties_t zes_fan_properties = {};
			zesFanGetState(zes_fan_handles[j], ZES_FAN_SPEED_UNITS_RPM, &gpu_fan);
			zesFanGetConfig(zes_fan_handles[j], &zes_fan_config);
			zesFanGetProperties(zes_fan_handles[j], &zes_fan_properties);
			gpus[g].fan_max = max(gpus[g].fan_max, zes_fan_properties.maxRPM>0 ? (uint)zes_fan_properties.maxRPM : 0u); // harden against reading dropouts
			const uint zes_fan_fpm = gpu_fan>=0 ? (uint)gpu_fan : zes_fan_config.speedFixed.speed>0 ? (zes_fan_config.speedFixed.units==ZES_FAN_SPEED_UNITS_RPM ? (uint)zes_fan_config.speedFixed.speed : zes_fan_config.speedFixed.units==ZES_FAN_SPEED_UNITS_PERCENT ? (gpus[g].fan_max*(uint)zes_fan_config.speedFixed.speed+50u)/100u: 0u) : 0u; // harden against unavailable counters
			gpu_fan_rpm = max(gpu_fan_rpm, zes_fan_fpm<2u*gpus[g].fan_max ? zes_fan_fpm : 0u); // harden against broken counters
		}
		delete[] zes_fan_handles;
		gpus[g].fan_current = gpu_fan_rpm;
		if(zes_fan_count==0u) gpus[g].fan_current = gpus[g].fan_max = max_uint; // no data available
		uint zes_freq_count = 0u;
		zesDeviceEnumFrequencyDomains(zes_device, &zes_freq_count, nullptr);
		zes_freq_handle_t* zes_freq_handles = new zes_freq_handle_t[zes_freq_count];
		zesDeviceEnumFrequencyDomains(zes_device, &zes_freq_count, zes_freq_handles);
		bool zes_freq_domain_memory_found = false;
		for(uint j=0u; j<zes_freq_count; j++) {
			zes_freq_properties_t zes_freq_properties = {};
			zesFrequencyGetProperties(zes_freq_handles[j], &zes_freq_properties);
			if(zes_freq_properties.type==ZES_FREQ_DOMAIN_GPU) {
				zes_freq_state_t zes_freq_state = {};
				zesFrequencyGetState(zes_freq_handles[j], &zes_freq_state);
				gpus[g].clock_core_current = zes_freq_state.actual>0.0 ? to_uint(zes_freq_state.actual) : 0.0;
			}
			if(zes_freq_properties.type==ZES_FREQ_DOMAIN_MEMORY) {
				zes_freq_state_t zes_freq_state = {};
				zesFrequencyGetState(zes_freq_handles[j], &zes_freq_state);
				gpus[g].clock_memory_current = zes_freq_state.actual>0.0 ? to_uint(zes_freq_state.actual<4000.0 ? zes_freq_state.actual : zes_freq_state.actual/(double)gpus[g].memory_transfers_per_clock) : 0.0; // zes_freq_state.actual may wrongly return value in MT/s instead of MHz, so divide by memory_transfers_per_clock
				zes_freq_domain_memory_found = true;
			}
		}
		delete[] zes_freq_handles;
		if(!zes_freq_domain_memory_found) gpus[g].clock_memory_current = gpus[g].memory_current>=200u||gpus[g].memory_bandwidth_current>0u ? gpus[g].clock_memory_max : 0u; // harden against unavailable counter
		zes_pci_stats_t zes_pci_stats = {};
		zes_pci_state_t zes_pci_state = {};
		zes_pci_properties_t zes_pci_properties = {};
		zesDevicePciGetStats(zes_device, &zes_pci_stats);
		zesDevicePciGetState(zes_device, &zes_pci_state); // broken on Linux
		zesDevicePciGetProperties(zes_device, &zes_pci_properties);
		const ulong zes_pcie_bandwidth_interval = zes_pci_stats.timestamp-zes_last_pci_timestamp[i];
		zes_available_pci[i] = zes_available_pci[i]||zes_pci_stats.txCounter+zes_pci_stats.rxCounter>0ull; // harden against reading dropouts
		gpus[g].pcie_bandwidth_current =  zes_available_pci[i] ? (zes_pcie_bandwidth_interval>0ull ? (uint)((zes_pci_stats.txCounter+zes_pci_stats.rxCounter-zes_last_pci[i]+zes_pcie_bandwidth_interval/2ull)/zes_pcie_bandwidth_interval) : gpus[g].pcie_bandwidth_current) : max_uint; // reuse last value if interval is 0
		const uint gpu_pcie_gen = zes_pci_stats.speed.gen>0 ? zes_pci_stats.speed.gen : zes_pci_state.speed.gen>0 ? zes_pci_state.speed.gen : gpus[g].pcie_bandwidth_max==0u&&zes_pci_properties.maxSpeed.gen>0 ? zes_pci_properties.maxSpeed.gen : 0u; // harden against broken counters
		const uint gpu_pcie_width = zes_pci_stats.speed.width>0 ? zes_pci_stats.speed.width : zes_pci_state.speed.width>0 ? zes_pci_state.speed.width : gpus[g].pcie_bandwidth_max==0u&&zes_pci_properties.maxSpeed.width>0 ? zes_pci_properties.maxSpeed.width : 0u; // harden against broken counters
		gpus[g].set_pcie_bandwidth_max(gpu_pcie_gen, gpu_pcie_width); // harden against load-dependent reading
		zes_last_pci[i] = zes_pci_stats.txCounter+zes_pci_stats.rxCounter;
		zes_last_pci_timestamp[i] = zes_pci_stats.timestamp;
	}
}
void gpu_finalize_intel() {
	zes_devices.clear();
	zes_last_active.clear();
	zes_last_active_timestamp.clear();
	zes_last_readwrite.clear();
	zes_last_readwrite_timestamp.clear();
	zes_last_energy.clear();
	zes_last_energy_timestamp.clear();
	zes_last_pci.clear();
	zes_last_pci_timestamp.clear();
	zes_available_readwrite.clear();
	zes_available_energy.clear();
	zes_available_pci.clear();
}
#endif // INTEL_GPU

void initialize_data() { // initialize data
	cpu_initialize();
#ifdef NVIDIA_GPU
	gpu_initialize_nvidia();
#endif // NVIDIA_GPU
#ifdef AMD_GPU
	gpu_initialize_amd();
#endif // AMD_GPU
#ifdef INTEL_GPU
	gpu_initialize_intel();
#endif // INTEL_GPU
}
void update_data() {
#ifdef NVIDIA_GPU
	gpu_update_nvidia();
#endif // NVIDIA_GPU
#ifdef AMD_GPU
	gpu_update_amd();
#endif // AMD_GPU
#ifdef INTEL_GPU
	gpu_update_intel();
#endif // INTEL_GPU
	cpu_update(); // update CPU data last as it sums over all GPU's PCIe bandwidth
}
void finalize_data() {
	cpu_finalize();
#ifdef NVIDIA_GPU
	gpu_finalize_nvidia();
#endif // NVIDIA_GPU
#ifdef AMD_GPU
	gpu_finalize_amd();
#endif // AMD_GPU
#ifdef INTEL_GPU
	gpu_finalize_intel();
#endif // INTEL_GPU
	cpu.usage_core_current.clear();
	gpus.clear();
}

void initialize_graphs() {
	graph_cpu_usage_core_current.resize(cpu.cores);
	graph_gpu_usage.resize(gpu_number);
	graph_gpu_memory_bandwidth.resize(gpu_number);
	graph_gpu_power.resize(gpu_number);
	graph_gpu_memory.resize(gpu_number);
}
void update_graphs() {
	for(uint c=0u; c<cpu.cores; c++) {
		graph_cpu_usage_core_current[c].insert(cpu.usage_core_current[c]<max_uint ? (uchar)cpu.usage_core_current[c] : max_uchar);
	}
	graph_cpu_memory.insert(cpu.get_memory()<max_uint ? (uchar)cpu.get_memory() : max_uchar);
	for(uint g=0u; g<gpu_number; g++) {
		graph_gpu_usage           [g].insert((uchar)gpus[g].get_usage()           );
		graph_gpu_memory_bandwidth[g].insert((uchar)gpus[g].get_memory_bandwidth());
		graph_gpu_power           [g].insert((uchar)gpus[g].get_power()           );
		graph_gpu_memory          [g].insert((uchar)gpus[g].get_memory()          );
	}
}
void finalize_graphs() {
	graph_cpu_usage_core_current.clear();
	graph_gpu_usage.clear();
	graph_gpu_memory_bandwidth.clear();
	graph_gpu_power.clear();
	graph_gpu_memory.clear();
}



string value_to_string(const uint x) {
	return x<max_uint ? to_string(x) : "?";
}
void print_percentage(const uint percentage, const string suffix) {
	int k = 4, colors[] = { color_green, color_yellow, color_orange, color_red };
	print(d3(percentage)+suffix, colors[clamp(((int)percentage*k+50)/100, 0, k-1)]);
}
void print_progress(uint width, const uint value_current, const uint value_max) {
	width = max(width, 2u);
	if(value_max==0u||value_max==max_uint) { // data is unavailable
		print("[");
		print(repeat("-", width-2u), color_gray);
		print("]");
	} else {
		uint k = 4u, colors[] = { color_green, color_yellow, color_orange, color_red };
		uint percentage = ::percentage(value_current, value_max);
		percentage = percentage==(uint)max_uchar ? 0u : percentage;
		const uint p = ((width-2u)*percentage+50u)/100u; // +50u for correct rounding
		print("[");
		print_no_reset(repeat("|", p)+repeat(" ", width-2u-p), (int)colors[clamp(k*percentage/100u, 0u, k-1u)]);
		print_no_reset("]", STANDARD_TEXTCOLOR);
	}
}
void print_progress_number(uint width, const uint value_current, const uint value_max, const string unit="%") {
	width = max(width, 2u);
	if(value_max==0u||value_max==max_uint) { // data is unavailable
		print("[");
		print(repeat("-", width-2u), color_gray);
		print("]");
	} else {
		uint k = 4u, colors[] = { color_green, color_yellow, color_orange, color_red };
		//uint k = 6u, colors[] = { color_dark_blue, color_magenta, color_red, color_orange, color_yellow, color_white };
		string v = value_to_string(value_current)+(unit!="%"&&width>19u ? " / "+to_string(value_max) : "")+unit;
		const uint l = max(width-2u, length(v));
		uint percentage = ::percentage(value_current, value_max);
		percentage = percentage==(uint)max_uchar ? 0u : percentage;
		const uint p = (l*percentage+50u)/100u; // +50u for correct rounding
		string s = repeat(" ", l-length(v))+v;
		if(value_current<max_uint) {
			for(uint i=0u; i<min(p, length(s)); i++) if(s[i]==' ') s[i] = '|';
		} else {
			for(uint i=0u; i<length(s); i++) if(s[i]==' ') s[i] = '-';
		}
		print("[");
		print(s.substr(0, p), (int)colors[clamp(k*percentage/100u, 0u, k-1u)]);
		print(s.substr(p), color_gray);
		print("]");
	}
}
void print_graph(const uint x, const uint y, const uint w, const uint h, const CircularBuffer<uchar, N>& graph, const int color, const uint max=100u, const string unit="%", const uint value_current=max_uint, const uint value_max=max_uint) {
	set_console_cursor(x, y);
	const bool valid = (graph[N-1u]<max_uchar);
	string v = (value_current<max_uint ? to_string(value_current) : (valid ? to_string((uint)graph[N-1u]) : "0"))+(value_max<max_uint ? " / "+to_string(value_max) : "")+unit;
	v = repeat(" ", w)+v+(length(v)+2u<=w ? " " : "");
	v = substring(v, length(v)-w);
	string fh=":", hh=".";
	for(uint i=0u; i<h; i++) {
		set_console_cursor(x, y+i);
		string line = "";
		for(uint j=0u; j<w; j++) {
			const float fx=(float)j/(float)w, fy=(float)(h-i)/(float)h, fyh=fy-0.5f/(float)h; // between 0 and 1
			float gy = 1.0f;
			if(valid) {
				if(w<=N) {
					const uint xa=j*N/w, xb=(j+1u)*N/w; // average of several graph values
					uint sum = 0u;
					for(uint x=xa; x<xb; x++) sum += (uint)graph[x];
					gy = (float)sum/((float)(xb-xa)*(float)max);
				} else {
					const uint xa=(uint)(fx*(float)N), xb=min(xa+1u, N-1u); // linear interpolation
					const float wb=(fx*(float)N)-(float)xa, wa=1.0f-wb;
					gy = (wa*(float)graph[xa]+wb*(float)graph[xb])/(float)max;
				}
			} else {
				fh = "/";
			}
			const string c = substring(v, j, 1u);
			line += i==0u ? (gy>=fy ? (c==" " ? fh : c) : gy>=fyh ? (c==" " ? hh : c) : c) : (gy>=fy ? fh : gy>=fyh ? hh : " ");
		}
		print(line, valid ? color : color_gray);
	}
}
int get_vendor_color(const char vendor) {
	return vendor=='N' ? 0x76B900 : vendor=='A' ? 0xED1C24 : vendor=='I' ? 0x0071C5 : 0xBFBFBF;
}
int get_vendor_color_ascii(const char vendor) {
	return vendor=='N' ? color_dark_green : vendor=='A' ? color_dark_red : vendor=='I' ? color_blue : color_gray;
}
void print_specs() {
	const int label_color = color_light_blue;
	print("CPU  ", label_color); print(": "); print(cpu.name, get_vendor_color_ascii(cpu.vendor)); println(" ("+to_string(cpu.cores)+" threads)");
	print("RAM  ", label_color); println(": "+value_to_string(cpu.memory_max) +" MB");
	print("PCIe ", label_color); println(": "+value_to_string(cpu.pcie_bandwidth_max) +" MB/s");
	for(uint g=0u; g<gpu_number; g++) {
		println();
		print("GPU "+to_string(g+1u), label_color); print(": "); println(gpus[g].name, get_vendor_color_ascii(gpus[g].vendor));
		print("VRAM ", label_color); println(": "+value_to_string(gpus[g].memory_max) +" MB @ "+to_string((gpus[g].memory_bandwidth_max+500u)/1000u)+" GB/s ("+value_to_string(gpus[g].memory_bus_width)+"-bit @ "+value_to_string(gpus[g].clock_memory_max*gpus[g].memory_transfers_per_clock)+" MT/s)");
		print("TDP  ", label_color); println(": "+value_to_string(gpus[g].power_max) +" W");
		print("Clock", label_color); println(": "+value_to_string(gpus[g].clock_core_max) +" MHz (core), "+value_to_string(gpus[g].clock_memory_max) +" MHz (memory)");
		print("PCIe ", label_color); println(": "+value_to_string(gpus[g].pcie_bandwidth_max)+" MB/s (PCIe "+value_to_string(gpus[g].pcie_gen_max)+".0 x"+value_to_string(gpus[g].pcie_width_max)+")");
	}
}
void print_data_bar(uint width, uint height) {
	const uint graphs = cpu.cores;
	uint rows = max(height, 6u+12u*(gpu_number>0u))-(5u+12u*(gpu_number>0u));
	while(graphs%rows!=0u) rows--;
	uint columns = graphs/rows;
	const uint label_width = 8u;
	width = max(width, label_width+11u);
	const uint k = lcm(max(gpu_number, 1u), columns);
	const uint d = max(width-label_width, k)/k-1u;
	width = label_width+k*(d+1u);
	const uint wf = (width-label_width)/1u-1u;
	const uint wc = (width-label_width)/columns-1u;
	const uint wg = (width-label_width)/max(gpu_number, 1u)-1u;
	show_console_cursor(false);
	set_console_cursor(0u, 0u);
	const int label_color = color_light_blue;
	print("    CPU ", label_color); print("["); print(substring(alignl(wf-2u, cpu.name), 0, wf-2u), get_vendor_color_ascii(cpu.vendor)); println("]");
	for(uint i=0; i<rows; i++) {
		if(columns>1u) {
			print(d3(i*columns+1u)+"-"+d3((i+1u)*columns)+" ", label_color);
		} else {
			print("    "+d3(i*columns+1u)+" ", label_color);
		}
		for(uint j=0u; j<columns; j++) {
			print_progress_number(wc, cpu.usage_core_current[i*columns+j], cpu.usage_max, "%"); if(j+1u<columns) print(" ");
		}
		println();
	}
	print(  "  Usage ", label_color); print_progress_number(wf, cpu.usage_current      , cpu.usage_max      ,  "%"  );
	print("\n    RAM ", label_color); print_progress_number(wf, cpu.memory_current     , cpu.memory_max     , " MB" );
	print("\n   Temp ", label_color); print_progress_number(wf, cpu.temperature_current, cpu.temperature_max, "'C"  );
	print("\n  Clock ", label_color); print_progress_number(wf, cpu.clock_current      , cpu.clock_max      , " MHz");
	if(gpu_number>0u) {
		print("\nPCIe BW ", label_color); print_progress_number(wf, cpu.pcie_bandwidth_current, cpu.pcie_bandwidth_max, " MB/s");
		print("\n"+repeat(" ", width-1u));
		print("\n    GPU ", label_color); for(uint g=0u; g<gpu_number; g++) { print("["); print(substring(alignl(wg-2u, gpus[g].name), 0u, wg-2u), get_vendor_color_ascii(gpus[g].vendor)); print("]"); if(g+1u<gpu_number) print(" "); }
		print("\n  Usage ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].usage_current         ,  gpus[g].usage_max         ,  "%"   ); if(g+1u<gpu_number) print(" "); }
		print("\nVRAM BW ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].memory_bandwidth_current<max_uint ? (gpus[g].memory_bandwidth_current+500u)/1000u : max_uint, gpus[g].memory_bandwidth_max<max_uint ? (gpus[g].memory_bandwidth_max+500u)/1000u : max_uint, " GB/s"); if(g+1u<gpu_number) print(" "); }
		print("\n   VRAM ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].memory_current        ,  gpus[g].memory_max        , " MB"  ); if(g+1u<gpu_number) print(" "); }
		print("\n   Temp ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].temperature_current   ,  gpus[g].temperature_max   , "'C"   ); if(g+1u<gpu_number) print(" "); }
		print("\n  Power ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].power_current         ,  gpus[g].power_max         , " W"   ); if(g+1u<gpu_number) print(" "); }
		print("\n    Fan ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].fan_current           ,  gpus[g].fan_max           , " RPM" ); if(g+1u<gpu_number) print(" "); }
		print("\n  Clock ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].clock_core_current    ,  gpus[g].clock_core_max    , " MHz" ); if(g+1u<gpu_number) print(" "); }
		print("\nMem Clk ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].clock_memory_current  ,  gpus[g].clock_memory_max  , " MHz" ); if(g+1u<gpu_number) print(" "); }
		print("\nPCIe BW ", label_color); for(uint g=0u; g<gpu_number; g++) { print_progress_number(wg, gpus[g].pcie_bandwidth_current,  gpus[g].pcie_bandwidth_max, " MB/s"); if(g+1u<gpu_number) print(" "); }
	}
	show_console_cursor(true);
}
void print_data_graph(uint width, uint height) {
	update_graphs();
	const bool has_gpus = gpu_number>0u;
	const uint graphs = cpu.cores; // for testing: n%64u+1u;
	const float aspect_ratio = 0.2f*(float)width/(float)height;
	uint rows = max((uint)(sqrt((float)graphs)/aspect_ratio), 1u); // align CPU cores in a grid
	while(graphs%rows!=0u) rows--;
	uint columns = graphs/rows;
	const uint columns_total = columns+1u+4u*(uint)has_gpus; // CPU columns + RAM + (GPU usage + GPU VRAM bandwidth + GPU power + GPU VRAM)
	const uint rows_total = lcm(max(gpu_number, 1u), rows);
	width = max(width, 2u*columns_total+1u);
	height = max(height, 2u*rows+1u);
	width = ((width-2u)/columns_total)*columns_total+1u; // make all graphs equally sized; -2u for last border + cursor
	height = ((height-1u)/rows_total)*rows_total+1u;
	const uint w_cpu  = (((width-1u)/columns_total-1u)+1u)*columns-1u;
	const uint w_gpu  = has_gpus ? (width-1u)/columns_total-1u : 0u;
	const uint w_gbw  = has_gpus ? (width-1u)/columns_total-1u : 0u;
	const uint w_gpow = has_gpus ? (width-1u)/columns_total-1u : 0u;
	const uint w_gmem = has_gpus ? (width-1u)/columns_total-1u : 0u;
	const uint w_cmem = (width-2u)-(w_cpu+1u)-(has_gpus ? ((w_gpu+1u)+(w_gbw+1u)+(w_gpow+1u)+(w_gmem+1u)) : 0u);
	const uint h_cpu  = height-2u;
	const uint h_cmem = height-2u;
	const uint h_gpu  = height-2u;
	const uint x_cpu  = 1u;
	const uint x_cmem = 1u+w_cpu+1u;
	const uint x_gpu  = 1u+w_cpu+1u+w_cmem+1u;
	const uint x_gbw  = 1u+w_cpu+1u+w_cmem+1u+w_gpu+1u;
	const uint x_gpow = 1u+w_cpu+1u+w_cmem+1u+w_gpu+1u+w_gbw+1u;
	const uint x_gmem = 1u+w_cpu+1u+w_cmem+1u+w_gpu+1u+w_gbw+1u+w_gpow+1u;
	const uint y_cpu  = 1u;
	const uint y_cmem = 1u;
	const uint y_gpu  = 1u;
	show_console_cursor(false);
	for(uint y=0u; y<rows; y++) {
		const uint y_graph = y_cpu+y*(h_cpu+1u)/rows;
		const uint h_graph = (y+1u)*(h_cpu+1u)/rows-y*(h_cpu+1u)/rows-1u;
		for(uint x=0u; x<columns; x++) {
			const uint c = x+y*columns;
			const uint x_graph = x_cpu+x*(w_cpu+1u)/columns;
			const uint w_graph = (x+1u)*(w_cpu+1u)/columns-x*(w_cpu+1u)/columns-1u;
			print_graph(x_graph, y_graph, w_graph, h_graph, graph_cpu_usage_core_current[c], color_cyan);
		}
	}
	for(uint y=0u; y<rows; y++) {
		const uint y_graph = y_cpu+y*(h_cpu+1u)/rows;
		const uint h_graph = (y+1u)*(h_cpu+1u)/rows-y*(h_cpu+1u)/rows-1u;
		for(uint x=0u; x+1u<columns; x++) {
			const uint x_graph = x_cpu+x*(w_cpu+1u)/columns;
			const uint w_graph = (x+1u)*(w_cpu+1u)/columns-x*(w_cpu+1u)/columns-1u;
			for(uint z=0u; z<h_graph; z++) {
				set_console_cursor(x_graph+w_graph, y_graph+z);
				print("|");
			}
		}
	}
	for(uint y=0u; y+1u<rows; y++) {
		const uint y_graph = y_cpu+y*(h_cpu+1u)/rows;
		const uint h_graph = (y+1u)*(h_cpu+1u)/rows-y*(h_cpu+1u)/rows-1u;
		for(uint x=0u; x+1u<columns; x++) {
			const uint x_graph = x_cpu+x*(w_cpu+1u)/columns;
			const uint w_graph = (x+1u)*(w_cpu+1u)/columns-x*(w_cpu+1u)/columns-1u;
			set_console_cursor(x_graph, y_graph+h_graph);
			print(repeat("-", w_graph)+"+");
		}
		const uint w_graph = (w_cpu+1u)-(columns-1u)*(w_cpu+1u)/columns-1u;
		set_console_cursor(x_cpu+(columns-1u)*(w_cpu+1u)/columns, y_graph+h_graph);
		print(repeat("-", w_graph));
	}
	for(uint x=0u; x<columns; x++) {
		const uint x_graph = x_cpu+x*(w_cpu+1u)/columns;
		const uint w_graph = (x+1u)*(w_cpu+1u)/columns-x*(w_cpu+1u)/columns-1u;
		set_console_cursor(x_graph-1u, y_cpu-1u);
		print("."+repeat("-", w_graph));
		set_console_cursor(x_graph-1u, y_cpu+h_cpu);
		print("'"+repeat("-", w_graph));
	}
	print_graph(x_cmem, y_cmem, w_cmem, h_cmem, graph_cpu_memory, color_magenta, 100u, " MB", cpu.memory_current);
	const uint lines = gpu_number;
	for(uint y=0u; y<lines; y++) {
		const uint y_graph = y_gpu+y*(h_gpu+1u)/lines;
		const uint h_graph = (y+1u)*(h_gpu+1u)/lines-y*(h_gpu+1u)/lines-1u;
		const uint g = y;
		const int gpu_color = get_vendor_color_ascii(gpus[g].vendor);
		print_graph(x_gpu, y_graph, w_gpu, h_graph, graph_gpu_usage[g], gpu_color, 100u);
		print_graph(x_gbw , y_graph, w_gbw , h_graph, graph_gpu_memory_bandwidth[g], gpu_color, 100u, " GB/s", (gpus[g].memory_bandwidth_current+500u)/1000u);
		if(w_gpow>=12u&&gpus[g].temperature_current>0u) {
			print_graph(x_gpow, y_graph, w_gpow, h_graph, graph_gpu_power[g], gpu_color, 100u, "'C "+alignr(3u, gpus[g].power_current<max_uint ? gpus[g].power_current : 0u)+" W", gpus[g].temperature_current);
		} else {
			print_graph(x_gpow, y_graph, w_gpow, h_graph, graph_gpu_power[g], gpu_color, 100u, " W", gpus[g].power_current);
		}
		print_graph(x_gmem, y_graph, w_gmem, h_graph, graph_gpu_memory[g], color_orange, 100u, " MB", gpus[g].memory_current);
	}
	for(uint y=0u; y<lines; y++) {
		const uint y_graph = y_gpu+y*(h_gpu+1u)/lines;
		const uint h_graph = (y+1u)*(h_gpu+1u)/lines-y*(h_gpu+1u)/lines-1u;
		for(uint z=0u; z<h_graph; z++) {
			set_console_cursor(x_gbw -1u, y_graph+z); print("|");
			set_console_cursor(x_gpow-1u, y_graph+z); print("|");
			set_console_cursor(x_gmem-1u, y_graph+z); print("|");
		}
	}
	for(uint y=0u; y+1u<lines; y++) {
		const uint y_graph = y_gpu+y*(h_gpu+1u)/lines;
		const uint h_graph = (y+1u)*(h_gpu+1u)/lines-y*(h_gpu+1u)/lines-1u;
		set_console_cursor(x_gpu, y_graph+h_graph);
		print(repeat("-", w_gpu)+"+"+repeat("-", w_gbw)+"+"+repeat("-", w_gpow)+"+"+repeat("-", w_gmem));
	}

	set_console_cursor(x_cmem-1u, 0u);
	print("."+repeat("-", w_cmem)+"."+(has_gpus ? repeat("-", w_gpu)+"."+repeat("-", w_gbw)+"."+repeat("-", w_gpow)+"."+repeat("-", w_gmem)+"." : ""));
	for(uint y=1u; y+1u<height; y++) {
		set_console_cursor(       0u, y); print("|");
		set_console_cursor(x_cpu -1u, y); print("|");
		set_console_cursor(x_cmem-1u, y); print("|");
		set_console_cursor(x_gpu -1u, y); print("|");
		if(has_gpus) {
			set_console_cursor(x_gmem+w_gmem, y); print("|");
		}
	}
	set_console_cursor(x_cmem-1u, height-1u);
	print("'"+repeat("-", w_cmem)+"'"+(has_gpus ? repeat("-", w_gpu)+"'"+repeat("-", w_gbw)+"'"+repeat("-", w_gpow)+"'"+repeat("-", w_gmem)+"'" : ""));
	show_console_cursor(true);
}



#ifndef WINDOWS_GRAPHICS

int main(int argc, char* argv[]) {
#if defined(_WIN32)
	SetConsoleTitle("hw-smi (c) Dr. Moritz Lehmann");
#elif defined(__linux__)
	struct sigaction act = { 0 };
	act.sa_sigaction = &handler;
	sigaction(SIGINT, &act, NULL);
#endif // Linux
	const vector<string> main_arguments = get_main_arguments(argc, argv);
	bool graphs = false;
	initialize_data();
	update_data();
	if(main_arguments.size()>0) {
		if(contains(main_arguments[0], "-g")) {
			graphs = true;
		} else if(contains(main_arguments[0], "-b")) {
			graphs = false;
		} else {
			println("hw-smi (c) Dr. Moritz Lehmann\nhttps://github.com/ProjectPhysX/hw-smi\n\nCommand-line options:\n -b / --bars  : visualize metrics as bars (default)\n -g / --graphs: visualize metrics as graphs\n -h / --help  : print this message\n\nHardware detected:\n");
			print_specs();
			print("\nPress Enter to continue.");
			wait();
		}
	}
	if(graphs) initialize_graphs();
	Clock clock;
	uint last_width=0u, last_height=0u;
	while(running) {
		clock.start();
		update_data();
		uint width=0u, height=0u;
		get_console_size(width, height);
		if(width!=last_width||height!=last_height) clear_console();
		last_width = width; last_height = height;
		if(graphs) {
			print_data_graph(width, height);
		} else {
			print_data_bar(width, height);
		}
		sleep(1.0/(double)UPDATE_FREQUENCY-clock.stop());
	}
	clear_console();
	set_console_cursor(0u, 0u);
	show_console_cursor(true);
	finalize_data();
	if(graphs) finalize_graphs();
	return 0;
}

#else // WINDOWS_GRAPHICS

void draw_graph(const uint x, const uint y, const uint w, const uint h, const CircularBuffer<uchar, N>& graph, const Color c, const uint grid=4u, const uint max=100u, const string unit="%", const int value=-1) {
	const uint x1=x, y1=y, x2=x+w-1u, y2=y+h-1u; // drawing area
	const bool valid = (graph[N-1u]<max_uchar);
	const Color cb = valid ? Color(63, 63, 63) : Color(c.r/2, c.g/2, c.b/2);
	/*const Color cb = Color(c.r/2u, c.g/2u, c.b/2u); // draw background grid
	const uint d=h/grid, h0=(h-d*grid)/2u;
	for(uint i=1u; i<grid; i++) {
		const uint yi = y+h0+i*d;
		draw_line(cb, x1, yi, x2, yi);
	}
	const uint wn = ::max((w/d+1u)*d, 1u);
	for(uint i=0u; i<=wn; i+=d) {
		const uint xi = x+(i+w-w*n/N)%wn;
		if(xi>x1&&xi<x2) draw_line(cb, xi, y1, xi, y2);
	}/**/
	if(valid) {
		const Color cb = Color(c.r/2, c.g/2, c.b/2);
		for(uint i=0u; i<N-1u; i++) { // first draw all polygons
			const uint xA = x1+(x2-x1)* i    /(N-1u);
			const uint xB = x1+(x2-x1)*(i+1u)/(N-1u);
			const uint yA = clamp(y2-(y2-y1)*(uint)graph[i  ]/max, y1, y2);
			const uint yB = clamp(y2-(y2-y1)*(uint)graph[i+1]/max, y1, y2);
			const int px[4] = { (int)xA, (int)xA, (int)xB, (int)xB };
			const int py[4] = { (int)y2, (int)yA, (int)yB, (int)y2 };
			draw_polygon(cb, px, py, 4);
		}
		for(uint i=0u; i<N-1u; i++) { // then draw all lines
			const uint xA = x1+(x2-x1)* i    /(N-1u);
			const uint xB = x1+(x2-x1)*(i+1u)/(N-1u);
			const uint yA = clamp(y2-(y2-y1)*(uint)graph[i]/max, y1, y2);
			const uint yB = clamp(y2-(y2-y1)*(uint)graph[i+1u]/max, y1, y2);
			draw_line(c, xA, yA, xB, yB);
		}
	} else {
		const Color cb = Color(47, 47, 47);
		const int px[4] = { (int)x1, (int)x2, (int)x2, (int)x1 };
		const int py[4] = { (int)y1, (int)y1, (int)y2, (int)y2 };
		draw_polygon(cb, px, py, 4);
	}
	draw_line(c, x1, y1, x1, y2); // then draw outlines and label
	draw_line(c, x2, y1, x2, y2);
	draw_line(c, x1, y1, x2, y1);
	draw_line(c, x1, y2, x2+1, y2);
	string v = "           "+to_string((value==-1 ? (uint)graph[N-1u] : value))+unit;
	draw_text(c, substring(v, length(v)-12u), x2-47u-3u*5u, y1-1u);
}

volatile bool initialized = false;
void main_physics() {
	initialize_data();
	initialize_graphs();
	initialized = true;
	Clock clock;
	while(running) {
		clock.start();
		update_data();
		sleep(1.0/(double)UPDATE_FREQUENCY-clock.stop());
	}
	finalize_data();
	finalize_graphs();
	PostMessage(GetConsoleWindow(), WM_CLOSE, 0, 0);
}
void main_graphics() {
	sleep(1.0/(double)UPDATE_FREQUENCY);
	if(!initialized) return;
	update_graphs();
	const bool has_gpus = gpu_number>0u;
	const uint graphs = cpu.cores;
	const float aspect_ratio = 0.2f*(float)camera.width/(float)camera.height;
	uint rows = max((uint)(sqrt((float)graphs)/aspect_ratio), 1u); // align CPU cores in a grid
	while(graphs%rows!=0u) rows--;
	uint columns = graphs/rows;
	const uint columns_total = columns+1u+4u*(uint)has_gpus;
	const uint w_cmem = camera.width*1u/columns_total;
	const uint w_gpu  = has_gpus ? camera.width*1u/columns_total : 0u;
	const uint w_gbw  = has_gpus ? camera.width*1u/columns_total : 0u;
	const uint w_gpow = has_gpus ? camera.width*1u/columns_total : 0u;
	const uint w_gmem = has_gpus ? camera.width*1u/columns_total : 0u;
	const uint w_cpu  = camera.width-w_cmem-w_gpu-w_gbw-w_gmem-w_gpow;
	const uint h_cpu  = camera.height;
	const uint h_cmem = camera.height;
	const uint h_gpu  = camera.height;
	const uint x_cpu  = 0u;
	const uint x_cmem = 0u+w_cpu;
	const uint x_gpu  = 0u+w_cpu+w_cmem;
	const uint x_gbw  = 0u+w_cpu+w_cmem+w_gpu;
	const uint x_gpow = 0u+w_cpu+w_cmem+w_gpu+w_gbw;
	const uint x_gmem = 0u+w_cpu+w_cmem+w_gpu+w_gbw+w_gpow;
	const uint y_cpu  = 0u;
	const uint y_cmem = 0u;
	const uint y_gpu  = 0u;
	for(uint y=0u; y<rows; y++) {
		const uint y_graph = y_cpu+y*h_cpu/rows;
		const uint h_graph = (y+1u)*h_cpu/rows-y*h_cpu/rows;
		for(uint x=0u; x<columns; x++) {
			uint one_pixel_x = x+1u<columns;
			uint one_pixel_y = y+1u<rows;
			const uint c = x+y*columns;
			const uint x_graph = x_cpu+x*w_cpu/columns;
			const uint w_graph = (x+1u)*w_cpu/columns-x*w_cpu/columns;
			draw_graph(x_graph, y_graph, w_graph+one_pixel_x, h_graph+one_pixel_y, graph_cpu_usage_core_current[c], Color( 26, 156, 234), max(4u/rows, 1u));
		}
	}
	draw_graph(x_cmem, y_cmem, w_cmem, h_cmem, graph_cpu_memory, Color(174,  51, 210), 4u, 100u, " MB", cpu.memory_current);
	const uint lines = gpu_number;
	for(uint y=0u; y<lines; y++) {
		const uint y_graph = y_gpu+y*h_gpu/lines;
		const uint h_graph = (y+1u)*h_gpu/lines-y*h_gpu/lines;
		const uint g = y;
		const int color = get_vendor_color(gpus[g].vendor);
		Color gpu_color(red(color), green(color), blue(color));
		if(w_gpu>=50u&&gpus[g].temperature_current>0u) {
			draw_graph(x_gpu , y_graph, w_gpu+1u, h_graph, graph_gpu_usage[g], gpu_color, 4u, 100u);
		} else {
			draw_graph(x_gpu , y_graph, w_gpu+1u, h_graph, graph_gpu_usage[g], gpu_color, 4u, 100u);
		}
		draw_graph(x_gbw , y_graph, w_gbw+1u, h_graph, graph_gpu_memory_bandwidth[g], gpu_color, 4u, 100u, " GB/s", (gpus[g].memory_bandwidth_current+500u)/1000u);
		draw_graph(x_gpow, y_graph, w_gpow  , h_graph, graph_gpu_power[g], gpu_color, 4u, 100u, "'C "+alignr(3u, gpus[g].power_current<max_uint ? gpus[g].power_current : 0u)+" W", gpus[g].temperature_current);
		draw_graph(x_gmem, y_graph, w_gmem  , h_graph, graph_gpu_memory[g], Color(220, 147,  12), 4u, 100u, " MB", gpus[g].memory_current);
	}
}

#endif // WINDOWS_GRAPHICS