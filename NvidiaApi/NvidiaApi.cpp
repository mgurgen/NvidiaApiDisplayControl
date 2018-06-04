#include <iostream>
#include "NvCpl.h"

#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

typedef enum
{
	NVAPI_THERMAL_TARGET_NONE = 0,
	NVAPI_THERMAL_TARGET_GPU = 1,     //!< GPU core temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_MEMORY = 2,     //!< GPU memory temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_POWER_SUPPLY = 4,     //!< GPU power supply temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_BOARD = 8,     //!< GPU board ambient temperature requires NvPhysicalGpuHandle
	NVAPI_THERMAL_TARGET_VCD_BOARD = 9,     //!< Visual Computing Device Board temperature requires NvVisualComputingDeviceHandle
	NVAPI_THERMAL_TARGET_VCD_INLET = 10,    //!< Visual Computing Device Inlet temperature requires NvVisualComputingDeviceHandle
	NVAPI_THERMAL_TARGET_VCD_OUTLET = 11,    //!< Visual Computing Device Outlet temperature requires NvVisualComputingDeviceHandle

	NVAPI_THERMAL_TARGET_ALL = 15,
	NVAPI_THERMAL_TARGET_UNKNOWN = -1,
} NV_THERMAL_TARGET;

typedef struct
{
	int   version;                //!< structure version 
	int   count;                  //!< number of associated thermal sensors
	struct
	{
		int       controller;        //!< internal, ADM1032, MAX6649...
		int                       defaultMinTemp;    //!< The min default temperature value of the thermal sensor in degree Celsius 
		int                       defaultMaxTemp;    //!< The max default temperature value of the thermal sensor in degree Celsius 
		int                       currentTemp;       //!< The current temperature value of the thermal sensor in degree Celsius 
		NV_THERMAL_TARGET           target;            //!< Thermal sensor targeted @ GPU, memory, chipset, powersupply, Visual Computing Device, etc.
	} sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS;

// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34

// function pointer types
typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int(*NvAPI_Initialize_t)();
typedef int(*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int(*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);
typedef int(*NvAPI_GPU_GetThermalSettings_t)(int *handle, int sensorIndex, NV_GPU_THERMAL_SETTINGS *temp);

int main()
{
	LPCSTR libraryName = "nvapi64.dll";
	HMODULE hmod = ::LoadLibrary(libraryName);
	if (hmod == NULL)
	{
		std::cerr << "Couldn't find nvapi.dll" << std::endl;
		return 1;
	}

	// nvapi.dll internal function pointers
	NvAPI_QueryInterface_t      NvAPI_QueryInterface = NULL;
	NvAPI_Initialize_t          NvAPI_Initialize = NULL;
	NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs = NULL;
	NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages = NULL;
	NvAPI_GPU_GetThermalSettings_t	NvAPI_GPU_GetThermalSettings = NULL;

	// nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
	NvAPI_QueryInterface = (NvAPI_QueryInterface_t)GetProcAddress(hmod, "nvapi_QueryInterface");

	// some useful internal functions that aren't exported by nvapi.dll
	NvAPI_Initialize = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
	NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
	NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);
	NvAPI_GPU_GetThermalSettings = (NvAPI_GPU_GetThermalSettings_t)(*NvAPI_QueryInterface)(0xE3640A56);

	if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
		NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL)
	{
		std::cerr << "Couldn't get functions in nvapi.dll" << std::endl;
		return 2;
	}

	// initialize NvAPI library, call it once before calling any other NvAPI functions
	(*NvAPI_Initialize)();
	NV_GPU_THERMAL_SETTINGS 		 nvgts;
	int          gpuCount = 0;
	int 		 gpuTemp;
	int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
	unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

	// gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
	gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

	(*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);
	nvgts.version = sizeof(NV_GPU_THERMAL_SETTINGS) | (1 << 16);
	nvgts.count = 0;
	nvgts.sensor[0].controller = -1;
	nvgts.sensor[0].target = NVAPI_THERMAL_TARGET_GPU;

	// print GPU usage every second
	for (int i = 0; i < 100; i++)
	{
		(*NvAPI_GPU_GetThermalSettings)(gpuHandles[0], 0, &nvgts);
		printf("%d\n", nvgts.sensor[0].currentTemp);
		(*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
		int usage = gpuUsages[3];
		std::cout << "GPU Usage: " << usage << std::endl;
		Sleep(1000);
	}

	return 0;
}

