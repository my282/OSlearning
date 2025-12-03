// clang-format off
#include <Uefi.h>
#include <Library/UefiLib.h>
// clang-format on
//俺はスマホからアクセスしてるぞ！ジョジョー
EFIAPI EFI_STATUS UefiMain(EFI_HANDLE ImageHandle,
                           EFI_SYSTEM_TABLE *SystemTable) {
  Print(L"Hello, UEFI World!\n");
  return EFI_SUCCESS;
}
