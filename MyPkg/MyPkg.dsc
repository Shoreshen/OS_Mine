# Must have , used to define variable assignments that can be used 
# in later build steps
[Defines]
    # Must have, .dsc file format version. 
    # Current newest release is 1.28 or 0x0001001C
    DSC_SPECIFICATION       = 0x0001001C
    # Required version and GUID, used to uniquely identify a platform file
    PLATFORM_GUID           = 62f4f74c-3951-4ada-b58c-1d3f4bd6a865
    PLATFORM_VERSION        = 0.1
    # Required string for name, used for default output dir as $(WORKSPAE)/Build/$(PLATFORM_NAME)
    PLATFORM_NAME           = My
    # Required, currently just DEFAULT
    SKUID_IDENTIFIER        = DEFAULT
    # Required, indicating supported architecture, use "|" to saperate
    SUPPORTED_ARCHITECTURES = X64
    # Required, indicating build target, use "|" to saperate
    BUILD_TARGETS           = DEBUG|RELEASE
    # Optional "FLASH_DEFINITION" also indicate the relative dir of the .fdf file

# Optional if there are no EDK II modules used by the DSC file
# Any reference listed in this package's module->.inf file->[LibraryClasses] section need to be listed here 
[LibraryClasses]
    # If in .inf file->[LibraryClasses] section listed UefiLib, the referencet module are $(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
    # The referenced module must be listed in relavent package->.dec->[LibraryClasses] section
    # For UefiLib, its listed under $(WORKSPACE)/MdePkg/MdePkg.dec:[LibraryClasses] section
    UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
    UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
    # If the referenced module reference other modules, it will also pop out a build error and indicating which module needed
    # The following are modules that referenced by module other than target module
    MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
    PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
    BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
    UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
    UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf


# Contain lists of EDK II Modules' .inf file
[Components]
    MyPkg/BootX64/BootX64.inf