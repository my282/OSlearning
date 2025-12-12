// clang-format off
#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include "ProcessorBind.h"
#include "Protocol/GraphicsOutput.h"
#include "Protocol/SimpleTextOut.h"
#include "Uefi/UefiBaseType.h"
#include "Uefi/UefiMultiPhase.h"
#include "Uefi/UefiSpec.h"
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/FileInfo.h>

struct MemoryMap {
  UINTN buffer_size;
  VOID *buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap *map) {

  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }
  map->map_size = map->buffer_size;
  return gBS->GetMemoryMap(
    &map->map_size,
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    &map->map_key,
    &map->descriptor_size,
    &map->descriptor_version
  );
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type){
  switch(type){
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    case EfiUnacceptedMemoryType: return L"EfiUnacceptedMemoryType";
    default: return L"InvalidMemoryType";
  }
}
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file){
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header =
    "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %016lx, map->map_size = %016lx\n",
        map->buffer, map->map_size);
  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for(iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i =0;
      iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
      iter += map->descriptor_size, i++){
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    len = AsciiSPrint(
      buf, sizeof(buf),
      "%2u, %x, %-25ls, %016lx, %lx, %lx\n",
      i, desc->Type, GetMemoryTypeUnicode(desc->Type),
      desc->PhysicalStart, desc->NumberOfPages,
      desc->Attribute & 0xffffflu);
    file->Write(file, &len, buf);
  }
  return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE ImageHandle, EFI_FILE_PROTOCOL** root){
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  EFI_STATUS status = gBS->OpenProtocol(
    ImageHandle,
  &gEfiLoadedImageProtocolGuid,
  (VOID**)&loaded_image,
  ImageHandle,
  NULL,
  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if(EFI_ERROR(status)) return status;

  EFI_STATUS status2 = gBS->OpenProtocol(
    loaded_image->DeviceHandle,
  &gEfiSimpleFileSystemProtocolGuid,
  (VOID**)&fs,
  ImageHandle,
  NULL,
  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if(EFI_ERROR(status2)) return status2;
  fs->OpenVolume(fs, root);
  return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE ImageHandle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL** gop){
  EFI_STATUS status;
  UINTN num_gop_handles = 0;
  EFI_HANDLE* gop_handles = NULL;

  status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    &num_gop_handles,
    &gop_handles
  );
  if(EFI_ERROR(status)){
    return status;
  }
  
  status = gBS->OpenProtocol(
    gop_handles[0],
    &gEfiGraphicsOutputProtocolGuid,
    (VOID**)gop,
    ImageHandle,
    NULL,
    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
  );
  if(EFI_ERROR(status)){
    return status;
  }
  FreePool(gop_handles);
  return EFI_SUCCESS;
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt){
  switch(fmt){
    case PixelRedGreenBlueReserved8BitPerColor:
      return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
      return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
      return L"PixelBitMask";
    case PixelBltOnly:
      return L"PixelBltOnly";
    case PixelFormatMax:
      return L"PixelFormatMax";
    default:
      return L"IncalidPixelFormat";
  }
}
EFIAPI EFI_STATUS UefiMain(EFI_HANDLE ImageHandle,
                            EFI_SYSTEM_TABLE *SystemTable) {
  CHAR8 memmap_buf[4096 * 4];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  EFI_FILE_PROTOCOL* root_dir;
  EFI_STATUS status;
  
  gST->ConOut->ClearScreen(gST->ConOut);
  Print(L"Hello, World!");
  status = OpenRootDir(ImageHandle, &root_dir);
  if(EFI_ERROR(status)) {
    Print(L"Error: OpenRootDir failed: %r\n", status);
    return status;
  }

  EFI_FILE_PROTOCOL* memmap_file;
  status = root_dir->Open(
    root_dir, &memmap_file, L"\\memmap",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0
  );
  if(EFI_ERROR(status)){
    Print(L"Error: Failed to open file \\memmap: %r\n", status);
    root_dir->Close(root_dir);
    return status;
  }

  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  OpenGOP(ImageHandle, &gop);
  Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
        gop->Mode->Info->PixelsPerScanLine);
  Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
        gop->Mode->FrameBufferSize);

  UINT32* frame_buffer = (UINT32*)gop->Mode->FrameBufferBase;
  for (UINTN i = 0; i < gop->Mode->FrameBufferSize /4; ++i){
    frame_buffer[i] = 0x004169e1;
  }

  gST->ConOut->ClearScreen(gST->ConOut);
  gST->ConOut->OutputString(gST->ConOut, L"Goodbye, Bootloader...\r\n");
  gBS->Stall(3000000);
  gST->ConOut->ClearScreen(gST->ConOut);

  EFI_FILE_PROTOCOL* kernel_file;
  root_dir->Open(
    root_dir, &kernel_file, L"\\kernel.elf",
    EFI_FILE_MODE_READ, 0
  );

  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12; //FileNameを格納するためのsizeof(CHAR16) * 12
  UINT8 file_info_buffer[file_info_size];
  kernel_file->GetInfo( //ファイルの情報を知る
    kernel_file, &gEfiFileInfoGuid,
    &file_info_size, file_info_buffer //EFI_FILE_INFO型を格納できる大きさのメモリ領域を指定(file_info_buffer)
  );

  EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
  UINTN kernel_file_size = file_info->FileSize;

  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  gBS->AllocatePages(
    AllocateAddress, EfiLoaderData,
    (kernel_file_size + 0xfff) / 0x1000, //切り上げ
    &kernel_base_addr
  );
  kernel_file->Read(kernel_file, &kernel_file_size, (VOID*)kernel_base_addr);
  Print(L"Kernel: 0x0lx (&lu bytes)/n", kernel_base_addr, kernel_file_size);

  EFI_STATUS exit_status = gBS->ExitBootServices(ImageHandle,memmap.map_key); //ブートサービスの停止に使用するステータス
  if(EFI_ERROR(exit_status)){
    exit_status = GetMemoryMap(&memmap);
    if(EFI_ERROR(exit_status)){
      Print(L"failed to get memory map:%r/\n", exit_status);
      while(1);
    }
    exit_status = gBS->ExitBootServices(ImageHandle,memmap.map_key);
    if(EFI_ERROR(exit_status)){
      Print(L"Could not exit boot service: %r\n", exit_status);
      while(1);
    }
  }
  UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

  typedef void EntryPointType(void);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  entry_point();
  return EFI_SUCCESS;
}
