#pragma once

typedef
PBUS_HANDLER
(FASTCALL* pHalHandlerForConfigSpace) (
    _In_ BUS_DATA_TYPE  ConfigSpace,
    _In_ ULONG          BusNumber
    );

typedef
VOID
(*pHalLocateHiberRanges)(
    _In_ PVOID MemoryMap
    );

typedef
NTSTATUS
(*PINSTALL_BUS_HANDLER)(
    IN PBUS_HANDLER   Bus
    );

typedef
NTSTATUS
(*pHalRegisterBusHandler)(
    _In_ INTERFACE_TYPE          InterfaceType,
    _In_ BUS_DATA_TYPE           AssociatedConfigurationSpace,
    _In_ ULONG                   BusNumber,
    _In_ INTERFACE_TYPE          ParentBusType,
    _In_ ULONG                   ParentBusNumber,
    _In_ ULONG                   SizeofBusExtensionData,
    _In_ PINSTALL_BUS_HANDLER    InstallBusHandlers,
    _Out_ PBUS_HANDLER* BusHandler
    );

typedef
VOID
(*pHalSetWakeEnable)(
    _In_ BOOLEAN          Enable
    );

typedef
VOID
(*pHalSetWakeAlarm)(
    _In_ ULONGLONG        WakeTime,
    _In_ PTIME_FIELDS     WakeTimeFields
    );

typedef
VOID
(*pHalLocateHiberRanges)(
    _In_ PVOID MemoryMap
    );

typedef
NTSTATUS
(*pHalAllocateMapRegisters)(
    _In_ struct _ADAPTER_OBJECT* DmaAdapter,
    _In_ ULONG NumberOfMapRegisters,
    _In_ ULONG BaseAddressCount,
    _Out_ PMAP_REGISTER_ENTRY MapRegsiterArray
    );

typedef struct {
    ULONG                               Version;

    pHalHandlerForBus                   HalHandlerForBus;
    pHalHandlerForConfigSpace           HalHandlerForConfigSpace;
    pHalLocateHiberRanges               HalLocateHiberRanges;

    pHalRegisterBusHandler              HalRegisterBusHandler;

    pHalSetWakeEnable                   HalSetWakeEnable;
    pHalSetWakeAlarm                    HalSetWakeAlarm;

    pHalTranslateBusAddress             HalPciTranslateBusAddress;
    pHalAssignSlotResources             HalPciAssignSlotResources;

    pHalHaltSystem                      HalHaltSystem;

    pHalFindBusAddressTranslation       HalFindBusAddressTranslation;

    pHalResetDisplay                    HalResetDisplay;

    pHalAllocateMapRegisters            HalAllocateMapRegisters;

    pKdSetupPciDeviceForDebugging       KdSetupPciDeviceForDebugging;
    pKdReleasePciDeviceForDebugging     KdReleasePciDeviceForDebugging;

    pKdGetAcpiTablePhase0               KdGetAcpiTablePhase0;
    pKdCheckPowerButton                 KdCheckPowerButton;

    pHalVectorToIDTEntry                HalVectorToIDTEntry;

    pKdMapPhysicalMemory64              KdMapPhysicalMemory64;
    pKdUnmapVirtualAddress              KdUnmapVirtualAddress;

} HAL_PRIVATE_DISPATCH, * PHAL_PRIVATE_DISPATCH;

typedef enum _INTERRUPT_MODEL {
    InterruptModelInvalid,
    InterruptModelApic,
    InterruptModelGic,
    InterruptModelBcm,
    InterruptModelUnknown = 0x1000
} INTERRUPT_MODEL, * PINTERRUPT_MODEL;

typedef
VOID
(*pHalAcpiTimerInterrupt)(
    VOID
    );

typedef struct {
    BOOLEAN     Supported;
    UCHAR       Pm1aVal;
    UCHAR       Pm1bVal;
} HAL_SLEEP_VAL, * PHAL_SLEEP_VAL;

typedef
VOID
(*pHalAcpiMachineStateInit)(
    _In_reads_(5) PHAL_SLEEP_VAL  SleepValues,
    _Out_ PINTERRUPT_MODEL InterruptModel
    );

typedef
ULONG
(*pHalAcpiQueryFlags)(
    VOID
    );

typedef
BOOLEAN
(*pHalPicStateIntact)(
    VOID
    );

typedef
VOID
(*pHalRestorePicState)(
    VOID
    );

typedef
ULONG
(*pHalInterfaceWriteConfig)(
    _In_ PVOID Context,
    _In_ ULONG BusOffset,
    _In_ ULONG Slot,
    _In_reads_bytes_(Buffer) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );

typedef
ULONG
(*pHalInterfaceReadConfig)(
    _In_ PVOID Context,
    _In_ ULONG BusOffset,
    _In_ ULONG Slot,
    _Out_writes_bytes_(Buffer) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length
    );

typedef
ULONG
(*pHalGetIOApicVersion)(
    _In_ ULONG BaseVector
    );

typedef
VOID
(*pHalSetMaxLegacyPciBusNumber)(
    _In_ ULONG BusNumber
    );

typedef
BOOLEAN
(*pHalIsVectorValid)(
    _In_ ULONG Vector
    );

typedef
PVOID
(*pHalGetAcpiTable)(
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId
    );

typedef
PVOID
(*pHalAcpiGetRsdp)(
    VOID
    );

typedef
PVOID
(*pHalAcpiGetFacsMapping)(
    VOID
    );

typedef
PVOID
(*pHalAcpiGetAllTables)(
    VOID
    );

//
// ACPI PM register enums.
//

typedef enum _ACPI_PM_REGISTER {
    PM1A_EVT_REGISTER = 0,
    PM1A_CTRL_REGISTER,
    PMTMR_REGISTER,
    PM1B_EVT_REGISTER,
    PM1B_CTRL_REGISTER,
    PM2_CTRL_REGISTER,
    PM_GP0_REGISTER,
    PM_GP1_REGISTER,
    PM_RESET_REGISTER,
    PM_REGISTER_END,
} ACPI_PM_REGISTER, * PACPI_PM_REGISTER;

typedef
NTSTATUS
(*pHalAcpiPmRegisterAvailable) (
    ACPI_PM_REGISTER Register
    );

typedef
NTSTATUS
(*pHalAcpiPmRegisterRead) (
    _In_ ACPI_PM_REGISTER Register,
    _In_ ULONG Offset,
    _Out_writes_bytes_(BufferSize) PVOID Output,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG BytesRead
    );

typedef
NTSTATUS
(*pHalAcpiPmRegisterWrite) (
    _In_ ACPI_PM_REGISTER Register,
    _In_ ULONG Offset,
    _In_reads_bytes_(BufferSize) PVOID Input,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG BytesWritten
    );

typedef struct {
    ULONG   Signature;
    ULONG   Version;
    pHalAcpiTimerInterrupt          HalpAcpiTimerInterrupt;
    pHalAcpiMachineStateInit        HalpAcpiMachineStateInit;
    pHalAcpiQueryFlags              HalpAcpiQueryFlags;
    pHalPicStateIntact              HalxPicStateIntact;
    pHalRestorePicState             HalxRestorePicState;
    pHalInterfaceReadConfig         HalpPciInterfaceReadConfig;
    pHalInterfaceWriteConfig        HalpPciInterfaceWriteConfig;
    pHalGetIOApicVersion            HalpGetIOApicVersion;
    pHalSetMaxLegacyPciBusNumber    HalpSetMaxLegacyPciBusNumber;
    pHalIsVectorValid               HalpIsVectorValid;
    pHalGetAcpiTable                HalpGetAcpiTable;
    pHalAcpiGetRsdp                 HalpAcpiGetRsdp;
    pHalAcpiGetFacsMapping          HalpAcpiGetFacsMapping;
    pHalAcpiGetAllTables            HalpAcpiGetAllTables;
    pHalAcpiPmRegisterAvailable     HalpAcpiPmRegisterAvailable;
    pHalAcpiPmRegisterRead          HalpAcpiPmRegisterRead;
    pHalAcpiPmRegisterWrite         HalpAcpiPmRegisterWrite;
} HAL_ACPI_DISPATCH_TABLE, * PHAL_ACPI_DISPATCH_TABLE;
