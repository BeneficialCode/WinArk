//#pragma once
//
//// rev
//// private
//typedef enum _SYSTEM_INFORMATION_CLASS
//{
//    SystemBasicInformation, // q: SYSTEM_BASIC_INFORMATION
//    SystemProcessorInformation, // q: SYSTEM_PROCESSOR_INFORMATION
//    SystemPerformanceInformation, // q: SYSTEM_PERFORMANCE_INFORMATION
//    SystemTimeOfDayInformation, // q: SYSTEM_TIMEOFDAY_INFORMATION
//    SystemPathInformation, // not implemented
//    SystemProcessInformation, // q: SYSTEM_PROCESS_INFORMATION
//    SystemCallCountInformation, // q: SYSTEM_CALL_COUNT_INFORMATION
//    SystemDeviceInformation, // q: SYSTEM_DEVICE_INFORMATION
//    SystemProcessorPerformanceInformation, // q: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
//    SystemFlagsInformation, // q: SYSTEM_FLAGS_INFORMATION
//    SystemCallTimeInformation, // not implemented // SYSTEM_CALL_TIME_INFORMATION // 10
//    SystemModuleInformation, // q: RTL_PROCESS_MODULES
//    SystemLocksInformation, // q: RTL_PROCESS_LOCKS
//    SystemStackTraceInformation, // q: RTL_PROCESS_BACKTRACES
//    SystemPagedPoolInformation, // not implemented
//    SystemNonPagedPoolInformation, // not implemented
//    SystemHandleInformation, // q: SYSTEM_HANDLE_INFORMATION
//    SystemObjectInformation, // q: SYSTEM_OBJECTTYPE_INFORMATION mixed with SYSTEM_OBJECT_INFORMATION
//    SystemPageFileInformation, // q: SYSTEM_PAGEFILE_INFORMATION
//    SystemVdmInstemulInformation, // q
//    SystemVdmBopInformation, // not implemented // 20
//    SystemFileCacheInformation, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypeSystemCache)
//    SystemPoolTagInformation, // q: SYSTEM_POOLTAG_INFORMATION
//    SystemInterruptInformation, // q: SYSTEM_INTERRUPT_INFORMATION
//    SystemDpcBehaviorInformation, // q: SYSTEM_DPC_BEHAVIOR_INFORMATION; s: SYSTEM_DPC_BEHAVIOR_INFORMATION (requires SeLoadDriverPrivilege)
//    SystemFullMemoryInformation, // not implemented
//    SystemLoadGdiDriverInformation, // s (kernel-mode only)
//    SystemUnloadGdiDriverInformation, // s (kernel-mode only)
//    SystemTimeAdjustmentInformation, // q: SYSTEM_QUERY_TIME_ADJUST_INFORMATION; s: SYSTEM_SET_TIME_ADJUST_INFORMATION (requires SeSystemtimePrivilege)
//    SystemSummaryMemoryInformation, // not implemented
//    SystemMirrorMemoryInformation, // s (requires license value "Kernel-MemoryMirroringSupported") (requires SeShutdownPrivilege) // 30
//    SystemPerformanceTraceInformation, // q; s: (type depends on EVENT_TRACE_INFORMATION_CLASS)
//    SystemObsolete0, // not implemented
//    SystemExceptionInformation, // q: SYSTEM_EXCEPTION_INFORMATION
//    SystemCrashDumpStateInformation, // s (requires SeDebugPrivilege)
//    SystemKernelDebuggerInformation, // q: SYSTEM_KERNEL_DEBUGGER_INFORMATION
//    SystemContextSwitchInformation, // q: SYSTEM_CONTEXT_SWITCH_INFORMATION
//    SystemRegistryQuotaInformation, // q: SYSTEM_REGISTRY_QUOTA_INFORMATION; s (requires SeIncreaseQuotaPrivilege)
//    SystemExtendServiceTableInformation, // s (requires SeLoadDriverPrivilege) // loads win32k only
//    SystemPrioritySeperation, // s (requires SeTcbPrivilege)
//    SystemVerifierAddDriverInformation, // s (requires SeDebugPrivilege) // 40
//    SystemVerifierRemoveDriverInformation, // s (requires SeDebugPrivilege)
//    SystemProcessorIdleInformation, // q: SYSTEM_PROCESSOR_IDLE_INFORMATION
//    SystemLegacyDriverInformation, // q: SYSTEM_LEGACY_DRIVER_INFORMATION
//    SystemCurrentTimeZoneInformation, // q; s: RTL_TIME_ZONE_INFORMATION
//    SystemLookasideInformation, // q: SYSTEM_LOOKASIDE_INFORMATION
//    SystemTimeSlipNotification, // s (requires SeSystemtimePrivilege)
//    SystemSessionCreate, // not implemented
//    SystemSessionDetach, // not implemented
//    SystemSessionInformation, // not implemented (SYSTEM_SESSION_INFORMATION)
//    SystemRangeStartInformation, // q: SYSTEM_RANGE_START_INFORMATION // 50
//    SystemVerifierInformation, // q: SYSTEM_VERIFIER_INFORMATION; s (requires SeDebugPrivilege)
//    SystemVerifierThunkExtend, // s (kernel-mode only)
//    SystemSessionProcessInformation, // q: SYSTEM_SESSION_PROCESS_INFORMATION
//    SystemLoadGdiDriverInSystemSpace, // s (kernel-mode only) (same as SystemLoadGdiDriverInformation)
//    SystemNumaProcessorMap, // q
//    SystemPrefetcherInformation, // q: PREFETCHER_INFORMATION; s: PREFETCHER_INFORMATION // PfSnQueryPrefetcherInformation
//    SystemExtendedProcessInformation, // q: SYSTEM_PROCESS_INFORMATION
//    SystemRecommendedSharedDataAlignment, // q
//    SystemComPlusPackage, // q; s
//    SystemNumaAvailableMemory, // 60
//    SystemProcessorPowerInformation, // q: SYSTEM_PROCESSOR_POWER_INFORMATION
//    SystemEmulationBasicInformation,
//    SystemEmulationProcessorInformation,
//    SystemExtendedHandleInformation, // q: SYSTEM_HANDLE_INFORMATION_EX
//    SystemLostDelayedWriteInformation, // q: ULONG
//    SystemBigPoolInformation, // q: SYSTEM_BIGPOOL_INFORMATION
//    SystemSessionPoolTagInformation, // q: SYSTEM_SESSION_POOLTAG_INFORMATION
//    SystemSessionMappedViewInformation, // q: SYSTEM_SESSION_MAPPED_VIEW_INFORMATION
//    SystemHotpatchInformation, // q; s: SYSTEM_HOTPATCH_CODE_INFORMATION
//    SystemObjectSecurityMode, // q: ULONG // 70
//    SystemWatchdogTimerHandler, // s (kernel-mode only)
//    SystemWatchdogTimerInformation, // q (kernel-mode only); s (kernel-mode only)
//    SystemLogicalProcessorInformation, // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION
//    SystemWow64SharedInformationObsolete, // not implemented
//    SystemRegisterFirmwareTableInformationHandler, // s (kernel-mode only)
//    SystemFirmwareTableInformation, // SYSTEM_FIRMWARE_TABLE_INFORMATION
//    SystemModuleInformationEx, // q: RTL_PROCESS_MODULE_INFORMATION_EX
//    SystemVerifierTriageInformation, // not implemented
//    SystemSuperfetchInformation, // q; s: SUPERFETCH_INFORMATION // PfQuerySuperfetchInformation
//    SystemMemoryListInformation, // q: SYSTEM_MEMORY_LIST_INFORMATION; s: SYSTEM_MEMORY_LIST_COMMAND (requires SeProfileSingleProcessPrivilege) // 80
//    SystemFileCacheInformationEx, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (same as SystemFileCacheInformation)
//    SystemThreadPriorityClientIdInformation, // s: SYSTEM_THREAD_CID_PRIORITY_INFORMATION (requires SeIncreaseBasePriorityPrivilege)
//    SystemProcessorIdleCycleTimeInformation, // q: SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION[]
//    SystemVerifierCancellationInformation, // not implemented // name:wow64:whNT32QuerySystemVerifierCancellationInformation
//    SystemProcessorPowerInformationEx, // not implemented
//    SystemRefTraceInformation, // q; s: SYSTEM_REF_TRACE_INFORMATION // ObQueryRefTraceInformation
//    SystemSpecialPoolInformation, // q; s (requires SeDebugPrivilege) // MmSpecialPoolTag, then MmSpecialPoolCatchOverruns != 0
//    SystemProcessIdInformation, // q: SYSTEM_PROCESS_ID_INFORMATION
//    SystemErrorPortInformation, // s (requires SeTcbPrivilege)
//    SystemBootEnvironmentInformation, // q: SYSTEM_BOOT_ENVIRONMENT_INFORMATION // 90
//    SystemHypervisorInformation, // q; s (kernel-mode only)
//    SystemVerifierInformationEx, // q; s: SYSTEM_VERIFIER_INFORMATION_EX
//    SystemTimeZoneInformation, // s (requires SeTimeZonePrivilege)
//    SystemImageFileExecutionOptionsInformation, // s: SYSTEM_IMAGE_FILE_EXECUTION_OPTIONS_INFORMATION (requires SeTcbPrivilege)
//    SystemCoverageInformation, // q; s // name:wow64:whNT32QuerySystemCoverageInformation; ExpCovQueryInformation
//    SystemPrefetchPatchInformation, // not implemented
//    SystemVerifierFaultsInformation, // s (requires SeDebugPrivilege)
//    SystemSystemPartitionInformation, // q: SYSTEM_SYSTEM_PARTITION_INFORMATION
//    SystemSystemDiskInformation, // q: SYSTEM_SYSTEM_DISK_INFORMATION
//    SystemProcessorPerformanceDistribution, // q: SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION // 100
//    SystemNumaProximityNodeInformation,
//    SystemDynamicTimeZoneInformation, // q; s (requires SeTimeZonePrivilege)
//    SystemCodeIntegrityInformation, // q: SYSTEM_CODEINTEGRITY_INFORMATION // SeCodeIntegrityQueryInformation
//    SystemProcessorMicrocodeUpdateInformation, // s
//    SystemProcessorBrandString, // q // HaliQuerySystemInformation -> HalpGetProcessorBrandString, info class 23
//    SystemVirtualAddressInformation, // q: SYSTEM_VA_LIST_INFORMATION[]; s: SYSTEM_VA_LIST_INFORMATION[] (requires SeIncreaseQuotaPrivilege) // MmQuerySystemVaInformation
//    SystemLogicalProcessorAndGroupInformation, // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX // since WIN7 // KeQueryLogicalProcessorRelationship
//    SystemProcessorCycleTimeInformation, // q: SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION[]
//    SystemStoreInformation, // q; s: SYSTEM_STORE_INFORMATION // SmQueryStoreInformation
//    SystemRegistryAppendString, // s: SYSTEM_REGISTRY_APPEND_STRING_PARAMETERS // 110
//    SystemAitSamplingValue, // s: ULONG (requires SeProfileSingleProcessPrivilege)
//    SystemVhdBootInformation, // q: SYSTEM_VHD_BOOT_INFORMATION
//    SystemCpuQuotaInformation, // q; s // PsQueryCpuQuotaInformation
//    SystemNativeBasicInformation, // not implemented
//    SystemSpare1, // not implemented
//    SystemLowPriorityIoInformation, // q: SYSTEM_LOW_PRIORITY_IO_INFORMATION
//    SystemTpmBootEntropyInformation, // q: TPM_BOOT_ENTROPY_NT_RESULT // ExQueryTpmBootEntropyInformation
//    SystemVerifierCountersInformation, // q: SYSTEM_VERIFIER_COUNTERS_INFORMATION
//    SystemPagedPoolInformationEx, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypePagedPool)
//    SystemSystemPtesInformationEx, // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypeSystemPtes) // 120
//    SystemNodeDistanceInformation,
//    SystemAcpiAuditInformation, // q: SYSTEM_ACPI_AUDIT_INFORMATION // HaliQuerySystemInformation -> HalpAuditQueryResults, info class 26
//    SystemBasicPerformanceInformation, // q: SYSTEM_BASIC_PERFORMANCE_INFORMATION // name:wow64:whNtQuerySystemInformation_SystemBasicPerformanceInformation
//    SystemQueryPerformanceCounterInformation, // q: SYSTEM_QUERY_PERFORMANCE_COUNTER_INFORMATION // since WIN7 SP1
//    SystemSessionBigPoolInformation, // q: SYSTEM_SESSION_POOLTAG_INFORMATION // since WIN8
//    SystemBootGraphicsInformation, // q; s: SYSTEM_BOOT_GRAPHICS_INFORMATION (kernel-mode only)
//    SystemScrubPhysicalMemoryInformation, // q; s: MEMORY_SCRUB_INFORMATION
//    SystemBadPageInformation,
//    SystemProcessorProfileControlArea, // q; s: SYSTEM_PROCESSOR_PROFILE_CONTROL_AREA
//    SystemCombinePhysicalMemoryInformation, // s: MEMORY_COMBINE_INFORMATION, MEMORY_COMBINE_INFORMATION_EX, MEMORY_COMBINE_INFORMATION_EX2 // 130
//    SystemEntropyInterruptTimingCallback,
//    SystemConsoleInformation, // q: SYSTEM_CONSOLE_INFORMATION
//    SystemPlatformBinaryInformation, // q: SYSTEM_PLATFORM_BINARY_INFORMATION
//    SystemThrottleNotificationInformation,
//    SystemHypervisorProcessorCountInformation, // q: SYSTEM_HYPERVISOR_PROCESSOR_COUNT_INFORMATION
//    SystemDeviceDataInformation, // q: SYSTEM_DEVICE_DATA_INFORMATION
//    SystemDeviceDataEnumerationInformation,
//    SystemMemoryTopologyInformation, // q: SYSTEM_MEMORY_TOPOLOGY_INFORMATION
//    SystemMemoryChannelInformation, // q: SYSTEM_MEMORY_CHANNEL_INFORMATION
//    SystemBootLogoInformation, // q: SYSTEM_BOOT_LOGO_INFORMATION // 140
//    SystemProcessorPerformanceInformationEx, // q: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX // since WINBLUE
//    SystemSpare0,
//    SystemSecureBootPolicyInformation, // q: SYSTEM_SECUREBOOT_POLICY_INFORMATION
//    SystemPageFileInformationEx, // q: SYSTEM_PAGEFILE_INFORMATION_EX
//    SystemSecureBootInformation, // q: SYSTEM_SECUREBOOT_INFORMATION
//    SystemEntropyInterruptTimingRawInformation,
//    SystemPortableWorkspaceEfiLauncherInformation, // q: SYSTEM_PORTABLE_WORKSPACE_EFI_LAUNCHER_INFORMATION
//    SystemFullProcessInformation, // q: SYSTEM_PROCESS_INFORMATION with SYSTEM_PROCESS_INFORMATION_EXTENSION (requires admin)
//    SystemKernelDebuggerInformationEx, // q: SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX
//    SystemBootMetadataInformation, // 150
//    SystemSoftRebootInformation, // q: ULONG
//    SystemElamCertificateInformation, // s: SYSTEM_ELAM_CERTIFICATE_INFORMATION
//    SystemOfflineDumpConfigInformation,
//    SystemProcessorFeaturesInformation, // q: SYSTEM_PROCESSOR_FEATURES_INFORMATION
//    SystemRegistryReconciliationInformation,
//    SystemEdidInformation,
//    SystemManufacturingInformation, // q: SYSTEM_MANUFACTURING_INFORMATION // since THRESHOLD
//    SystemEnergyEstimationConfigInformation, // q: SYSTEM_ENERGY_ESTIMATION_CONFIG_INFORMATION
//    SystemHypervisorDetailInformation, // q: SYSTEM_HYPERVISOR_DETAIL_INFORMATION
//    SystemProcessorCycleStatsInformation, // q: SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION // 160
//    SystemVmGenerationCountInformation,
//    SystemTrustedPlatformModuleInformation, // q: SYSTEM_TPM_INFORMATION
//    SystemKernelDebuggerFlags, // SYSTEM_KERNEL_DEBUGGER_FLAGS
//    SystemCodeIntegrityPolicyInformation, // q: SYSTEM_CODEINTEGRITYPOLICY_INFORMATION
//    SystemIsolatedUserModeInformation, // q: SYSTEM_ISOLATED_USER_MODE_INFORMATION
//    SystemHardwareSecurityTestInterfaceResultsInformation,
//    SystemSingleModuleInformation, // q: SYSTEM_SINGLE_MODULE_INFORMATION
//    SystemAllowedCpuSetsInformation,
//    SystemVsmProtectionInformation, // q: SYSTEM_VSM_PROTECTION_INFORMATION (previously SystemDmaProtectionInformation)
//    SystemInterruptCpuSetsInformation, // q: SYSTEM_INTERRUPT_CPU_SET_INFORMATION // 170
//    SystemSecureBootPolicyFullInformation, // q: SYSTEM_SECUREBOOT_POLICY_FULL_INFORMATION
//    SystemCodeIntegrityPolicyFullInformation,
//    SystemAffinitizedInterruptProcessorInformation,
//    SystemRootSiloInformation, // q: SYSTEM_ROOT_SILO_INFORMATION
//    SystemCpuSetInformation, // q: SYSTEM_CPU_SET_INFORMATION // since THRESHOLD2
//    SystemCpuSetTagInformation, // q: SYSTEM_CPU_SET_TAG_INFORMATION
//    SystemWin32WerStartCallout,
//    SystemSecureKernelProfileInformation, // q: SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION
//    SystemCodeIntegrityPlatformManifestInformation, // q: SYSTEM_SECUREBOOT_PLATFORM_MANIFEST_INFORMATION // since REDSTONE
//    SystemInterruptSteeringInformation, // 180
//    SystemSupportedProcessorArchitectures,
//    SystemMemoryUsageInformation, // q: SYSTEM_MEMORY_USAGE_INFORMATION
//    SystemCodeIntegrityCertificateInformation, // q: SYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION
//    SystemPhysicalMemoryInformation, // q: SYSTEM_PHYSICAL_MEMORY_INFORMATION // since REDSTONE2
//    SystemControlFlowTransition,
//    SystemKernelDebuggingAllowed, // s: ULONG
//    SystemActivityModerationExeState, // SYSTEM_ACTIVITY_MODERATION_EXE_STATE
//    SystemActivityModerationUserSettings, // SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS
//    SystemCodeIntegrityPoliciesFullInformation,
//    SystemCodeIntegrityUnlockInformation, // SYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION // 190
//    SystemIntegrityQuotaInformation,
//    SystemFlushInformation, // q: SYSTEM_FLUSH_INFORMATION
//    SystemProcessorIdleMaskInformation, // q: ULONG_PTR // since REDSTONE3
//    SystemSecureDumpEncryptionInformation,
//    SystemWriteConstraintInformation, // SYSTEM_WRITE_CONSTRAINT_INFORMATION
//    SystemKernelVaShadowInformation, // SYSTEM_KERNEL_VA_SHADOW_INFORMATION
//    SystemHypervisorSharedPageInformation, // SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION // since REDSTONE4
//    SystemFirmwareBootPerformanceInformation,
//    SystemCodeIntegrityVerificationInformation, // SYSTEM_CODEINTEGRITYVERIFICATION_INFORMATION
//    SystemFirmwarePartitionInformation, // SYSTEM_FIRMWARE_PARTITION_INFORMATION // 200
//    SystemSpeculationControlInformation, // SYSTEM_SPECULATION_CONTROL_INFORMATION // (CVE-2017-5715) REDSTONE3 and above.
//    SystemDmaGuardPolicyInformation, // SYSTEM_DMA_GUARD_POLICY_INFORMATION
//    SystemEnclaveLaunchControlInformation, // SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION
//    SystemWorkloadAllowedCpuSetsInformation, // SYSTEM_WORKLOAD_ALLOWED_CPU_SET_INFORMATION // since REDSTONE5
//    SystemCodeIntegrityUnlockModeInformation,
//    SystemLeapSecondInformation, // SYSTEM_LEAP_SECOND_INFORMATION
//    SystemFlags2Information, // q: SYSTEM_FLAGS_INFORMATION
//    SystemSecurityModelInformation, // SYSTEM_SECURITY_MODEL_INFORMATION // since 19H1
//    SystemCodeIntegritySyntheticCacheInformation,
//    MaxSystemInfoClass
//} SYSTEM_INFORMATION_CLASS;
//
//// private
//typedef enum _SYSDBG_COMMAND
//{
//    SysDbgQueryModuleInformation,
//    SysDbgQueryTraceInformation,
//    SysDbgSetTracepoint,
//    SysDbgSetSpecialCall,
//    SysDbgClearSpecialCalls,
//    SysDbgQuerySpecialCalls,
//    SysDbgBreakPoint,
//    SysDbgQueryVersion,
//    SysDbgReadVirtual,
//    SysDbgWriteVirtual,
//    SysDbgReadPhysical,
//    SysDbgWritePhysical,
//    SysDbgReadControlSpace,
//    SysDbgWriteControlSpace,
//    SysDbgReadIoSpace,
//    SysDbgWriteIoSpace,
//    SysDbgReadMsr,
//    SysDbgWriteMsr,
//    SysDbgReadBusData,
//    SysDbgWriteBusData,
//    SysDbgCheckLowMemory,
//    SysDbgEnableKernelDebugger,
//    SysDbgDisableKernelDebugger,
//    SysDbgGetAutoKdEnable,
//    SysDbgSetAutoKdEnable,
//    SysDbgGetPrintBufferSize,
//    SysDbgSetPrintBufferSize,
//    SysDbgGetKdUmExceptionEnable,
//    SysDbgSetKdUmExceptionEnable,
//    SysDbgGetTriageDump,
//    SysDbgGetKdBlockEnable,
//    SysDbgSetKdBlockEnable,
//    SysDbgRegisterForUmBreakInfo,
//    SysDbgGetUmBreakPid,
//    SysDbgClearUmBreakPid,
//    SysDbgGetUmAttachPid,
//    SysDbgClearUmAttachPid,
//    SysDbgGetLiveKernelDump
//} SYSDBG_COMMAND, * PSYSDBG_COMMAND;
//
//// begin_private
//
//typedef struct _PS_ATTRIBUTE
//{
//    ULONG_PTR Attribute;
//    SIZE_T Size;
//    union
//    {
//        ULONG_PTR Value;
//        PVOID ValuePtr;
//    };
//    PSIZE_T ReturnLength;
//} PS_ATTRIBUTE, * PPS_ATTRIBUTE;
//
//typedef struct _PS_ATTRIBUTE_LIST
//{
//    SIZE_T TotalLength;
//    PS_ATTRIBUTE Attributes[1];
//} PS_ATTRIBUTE_LIST, * PPS_ATTRIBUTE_LIST;
//
//extern "C" {
//    // ntoskrnl
//	NTSTATUS NTAPI ZwQueryInformationProcess(
//		_In_ HANDLE ProcessHandle,
//		_In_ PROCESSINFOCLASS ProcessInformationClass,
//		_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
//		_In_ ULONG ProcessInformationLength,
//		_Out_opt_ PULONG ReturnLength
//	);
//
//	NTSTATUS NTAPI ZwQuerySystemInformation(
//		_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
//		_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
//		_In_ ULONG SystemInformationLength,
//		_Out_opt_ PULONG ReturnLength
//	);
//
//    NTSTATUS NTAPI ZwQueryInformationThread(
//        _In_ HANDLE ThreadHandle,
//        _In_ THREADINFOCLASS ThreadInformationClass,
//        _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
//        _In_ ULONG ThreadInformationLength,
//        _Out_opt_ PULONG ReturnLength
//    );
//
//    NTSTATUS NTAPI KeRaiseUserException(
//        _In_ NTSTATUS ExceptionCode
//    );
//
//    NTSTATUS NTAPI ZwSetInformationProcess(
//        _In_ HANDLE ProcessHandle,
//        _In_ PROCESSINFOCLASS ProcessInformationClass,
//        _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
//        _In_ ULONG ProcessInformationLength
//    );
//
//    NTSTATUS NTAPI ZwQueryInformationProcess(
//        _In_ HANDLE ProcessHandle,
//        _In_ PROCESSINFOCLASS ProcessInformationClass,
//        _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
//        _In_ ULONG ProcessInformationLength,
//        _Out_opt_ PULONG ReturnLength
//    );
//    
//    NTSTATUS NTAPI ZwGetContextThread(
//        _In_ HANDLE ThreadHandle,
//        _Inout_ PCONTEXT ThreadContext
//    );
//
//    NTSTATUS NTAPI NtSetContextThread(
//        _In_ HANDLE ThreadHandle,
//        _In_ PCONTEXT ThreadContext
//    );
//
//    NTSTATUS NTAPI NtContinue(
//        _In_ PCONTEXT ContextRecord,
//        _In_ BOOLEAN TestAlert
//    );
//
//    NTSTATUS NTAPI NtSystemDebugControl(
//        _In_ SYSDBG_COMMAND Command,
//        _Inout_updates_bytes_opt_(InputBufferLength) PVOID InputBuffer,
//        _In_ ULONG InputBufferLength,
//        _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
//        _In_ ULONG OutputBufferLength,
//        _Out_opt_ PULONG ReturnLength
//    );
//
//    NTSTATUS NTAPI NtCreateThreadEx(
//        _Out_ PHANDLE ThreadHandle,
//        _In_ ACCESS_MASK DesiredAccess,
//        _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
//        _In_ HANDLE ProcessHandle,
//        _In_ PVOID StartRoutine, // PUSER_THREAD_START_ROUTINE
//        _In_opt_ PVOID Argument,
//        _In_ ULONG CreateFlags, // THREAD_CREATE_FLAGS_*
//        _In_ SIZE_T ZeroBits,
//        _In_ SIZE_T StackSize,
//        _In_ SIZE_T MaximumStackSize,
//        _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
//    );
//
//    NTSTATUS NTAPI NtTerminateThread(
//        _In_opt_ HANDLE ThreadHandle,
//        _In_ NTSTATUS ExitStatus
//    );
//
//    NTSTATUS NTAPI NtDuplicateObject(
//        _In_ HANDLE SourceProcessHandle,
//        _In_ HANDLE SourceHandle,
//        _In_opt_ HANDLE TargetProcessHandle,
//        _Out_opt_ PHANDLE TargetHandle,
//        _In_ ACCESS_MASK DesiredAccess,
//        _In_ ULONG HandleAttributes,
//        _In_ ULONG Options
//    );
//}