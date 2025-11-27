TARGET = BOOTX64.EFI
OBJS = main.o

CC = gcc
LD = ld
OBJCOPY = objcopy

# GNU-EFIのパス設定
EFIINC = /usr/include/efi
EFIINCS = -I$(EFIINC) -I$(EFIINC)/x86_64 -I$(EFIINC)/protocol
EFILIB = /usr/lib
GNUEFI_LIB = /usr/lib

# コンパイルフラグ
CFLAGS = $(EFIINCS) -fno-stack-protector -fpic \
         -fshort-wchar -mno-red-zone -Wall \
         -DGNU_EFI_USE_MS_ABI -ffreestanding

# リンクフラグ
LDFLAGS = -nostdlib -znocombreloc -T $(GNUEFI_LIB)/elf_x86_64_efi.lds -shared \
          -Bsymbolic -L$(EFILIB) -L$(GNUEFI_LIB) $(GNUEFI_LIB)/crt0-efi-x86_64.o

LIBS = -lefi -lgnuefi

all: $(TARGET)

# .o ファイルの生成
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# .so ファイルの生成
main.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

# .efi ファイルの生成
$(TARGET): main.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	           -j .dynsym -j .rel -j .rela -j .reloc \
	           --target=efi-app-x86_64 $< $@

clean:
	rm -f $(OBJS) main.so $(TARGET)

.PHONY: all clean
