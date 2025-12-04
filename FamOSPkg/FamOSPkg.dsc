[Defines]
  PLATFORM_NAME           = MyPkg
  PLATFORM_GUID           = 87654321-4321-4321-4321-CBA987654321
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/MyPkg
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = DEBUG|RELEASE

[LibraryClasses]
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

[Components]
  MyPkg/HelloWorld/HelloWorld.inf
