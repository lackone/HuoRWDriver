#pragma once
#include <ntifs.h>

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemMirrorMemoryInformation,
	SystemPerformanceTraceInformation,
	SystemObsolete0,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemVerifierAddDriverInformation,
	SystemVerifierRemoveDriverInformation,
	SystemProcessorIdleInformation,
	SystemLegacyDriverInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation,
	SystemTimeSlipNotification,
	SystemSessionCreate,
	SystemSessionDetach,
	SystemSessionInformation,
	SystemRangeStartInformation,
	SystemVerifierInformation,
	SystemVerifierThunkExtend,
	SystemSessionProcessInformation,
	SystemLoadGdiDriverInSystemSpace,
	SystemNumaProcessorMap,
	SystemPrefetcherInformation,
	SystemExtendedProcessInformation,
	SystemRecommendedSharedDataAlignment,
	SystemComPlusPackage,
	SystemNumaAvailableMemory,
	SystemProcessorPowerInformation,
	SystemEmulationBasicInformation,
	SystemEmulationProcessorInformation,
	SystemExtendedHandleInformation,
	SystemLostDelayedWriteInformation,
	SystemBigPoolInformation,
	SystemSessionPoolTagInformation,
	SystemSessionMappedViewInformation,
	SystemHotpatchInformation,
	SystemObjectSecurityMode,
	SystemWatchdogTimerHandler,
	SystemWatchdogTimerInformation,
	SystemLogicalProcessorInformation,
	SystemWow64SharedInformation,
	SystemRegisterFirmwareTableInformationHandler,
	SystemFirmwareTableInformation,
	SystemModuleInformationEx,
	SystemVerifierTriageInformation,
	SystemSuperfetchInformation,
	SystemMemoryListInformation,
	SystemFileCacheInformationEx,
	MaxSystemInfoClass  // MaxSystemInfoClass should always be the last enum
} SYSTEM_INFORMATION_CLASS;

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

typedef struct _KLDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	PVOID ExceptionTable;
	ULONG ExceptionTableSize;
	// ULONG padding on IA64
	PVOID GpValue;
	PVOID NonPagedDebugInfo;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT __Unused5;
	PVOID SectionPointer;
	ULONG CheckSum;
	// ULONG padding on IA64
	PVOID LoadedImports;
	PVOID PatchInformation;
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;

#pragma pack(push, 4)

typedef struct _PEB_LDR_DATA32
{
	ULONG Length;                                                           //0x0
	UCHAR Initialized;                                                      //0x4
	ULONG SsHandle;                                                         //0x8
	LIST_ENTRY32 InLoadOrderModuleList;										//0xc
	LIST_ENTRY32 InMemoryOrderModuleList;									//0x14
	LIST_ENTRY32 InInitializationOrderModuleList;							//0x1c
	ULONG EntryInProgress;                                                  //0x24
	UCHAR ShutdownInProgress;                                               //0x28
	ULONG ShutdownThreadId;                                                 //0x2c
} PEB_LDR_DATA32, * PPEB_LDR_DATA32;

typedef struct _PEB32
{
	UCHAR InheritedAddressSpace;                                            //0x0
	UCHAR ReadImageFileExecOptions;                                         //0x1
	UCHAR BeingDebugged;                                                    //0x2
	union
	{
		UCHAR BitField;                                                     //0x3
		struct
		{
			UCHAR ImageUsesLargePages : 1;                                  //0x3
			UCHAR IsProtectedProcess : 1;                                   //0x3
			UCHAR IsLegacyProcess : 1;                                      //0x3
			UCHAR IsImageDynamicallyRelocated : 1;                          //0x3
			UCHAR SkipPatchingUser32Forwarders : 1;                         //0x3
			UCHAR SpareBits : 3;                                            //0x3
		};
	};
	ULONG Mutant;                                                           //0x4
	ULONG ImageBaseAddress;                                                 //0x8
	ULONG Ldr;																//0xc
} PEB32, * PPEB32;

typedef struct _LDR_DATA_TABLE_ENTRY32
{
	LIST_ENTRY32 InLoadOrderLinks;											//0x0
	LIST_ENTRY32 InMemoryOrderLinks;										//0x8
	LIST_ENTRY32 InInitializationOrderLinks;								//0x10
	ULONG DllBase;                                                          //0x18
	ULONG EntryPoint;                                                       //0x1c
	ULONG SizeOfImage;                                                      //0x20
	UNICODE_STRING32 FullDllName;											//0x24
	UNICODE_STRING32 BaseDllName;											//0x2c
	ULONG Flags;                                                            //0x34
	USHORT LoadCount;                                                       //0x38
	USHORT TlsIndex;                                                        //0x3a
	union
	{
		LIST_ENTRY32 HashLinks;												//0x3c
		struct
		{
			ULONG SectionPointer;                                           //0x3c
			ULONG CheckSum;                                                 //0x40
		};
	};
	union
	{
		ULONG TimeDateStamp;                                                //0x44
		ULONG LoadedImports;                                                //0x44
	};
	ULONG EntryPointActivationContext;										//0x48
	ULONG PatchInformation;                                                 //0x4c
	LIST_ENTRY32 ForwarderLinks;											//0x50
	LIST_ENTRY32 ServiceTagLinks;											//0x58
	LIST_ENTRY32 StaticLinks;												//0x60
	ULONG ContextInformation;                                               //0x68
	ULONG OriginalBase;                                                     //0x6c
	LARGE_INTEGER LoadTime;													//0x70
} LDR_DATA_TABLE_ENTRY32, * PLDR_DATA_TABLE_ENTRY32;

#pragma pack(pop)

typedef struct _PEB_LDR_DATA
{
	ULONG Length;                                                           //0x0
	UCHAR Initialized;                                                      //0x4
	VOID* SsHandle;                                                         //0x8
	LIST_ENTRY InLoadOrderModuleList;										//0x10
	LIST_ENTRY InMemoryOrderModuleList;										//0x20
	LIST_ENTRY InInitializationOrderModuleList;								//0x30
	VOID* EntryInProgress;                                                  //0x40
	UCHAR ShutdownInProgress;                                               //0x48
	VOID* ShutdownThreadId;                                                 //0x50
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _PEB
{
	ULONG64 x;
	VOID* Mutant;                                                           //0x8
	VOID* ImageBaseAddress;                                                 //0x10
	PEB_LDR_DATA* Ldr;														//0x18
} PEB, * PPEB;

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;											//0x0
	LIST_ENTRY InMemoryOrderLinks;											//0x10
	LIST_ENTRY InInitializationOrderLinks;									//0x20
	VOID* DllBase;                                                          //0x30
	VOID* EntryPoint;                                                       //0x38
	ULONG64 SizeOfImage;                                                    //0x40
	UNICODE_STRING FullDllName;												//0x48
	UNICODE_STRING BaseDllName;												//0x58
	ULONG Flags;                                                            //0x68
	USHORT LoadCount;                                                       //0x6c
	USHORT TlsIndex;                                                        //0x6e
	union
	{
		LIST_ENTRY HashLinks;												//0x70
		struct
		{
			VOID* SectionPointer;                                           //0x70
			ULONG CheckSum;                                                 //0x78
		};
	};
	union
	{
		ULONG TimeDateStamp;                                                //0x80
		VOID* LoadedImports;                                                //0x80
	};
	struct _ACTIVATION_CONTEXT* EntryPointActivationContext;                //0x88
	VOID* PatchInformation;                                                 //0x90
	LIST_ENTRY ForwarderLinks;												//0x98
	LIST_ENTRY ServiceTagLinks;												//0xa8
	LIST_ENTRY StaticLinks;													//0xb8
	VOID* ContextInformation;                                               //0xc8
	ULONGLONG OriginalBase;                                                 //0xd0
	LARGE_INTEGER LoadTime;													//0xd8
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

NTSTATUS NTAPI ZwQuerySystemInformation(
	__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
	__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
	__in ULONG SystemInformationLength,
	__out_opt PULONG ReturnLength
);

EXTERN_C PVOID NTAPI PsGetProcessWow64Process(PEPROCESS Process);

EXTERN_C PVOID NTAPI PsGetProcessPeb(PEPROCESS Process);

EXTERN_C PVOID NTAPI PsGetThreadTeb(PETHREAD Thread);

NTSTATUS ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID* Object
);

NTSTATUS MmCopyVirtualMemory(
	IN PEPROCESS FromProcess,
	IN CONST VOID* FromAddress,
	IN PEPROCESS ToProcess,
	OUT PVOID ToAddress,
	IN SIZE_T BufferSize,
	IN KPROCESSOR_MODE PreviousMode,
	OUT PSIZE_T NumberOfBytesCopied
);