clang -target x86_64-pc-win32-coff -I/usr/include/efi \
    -mno-red-zone -fno-stack-protector -fshort-wchar -Wall -c main.c
lld-link /libpath:\usr\include\efi /subsystem:efi_application /entry:efi_main /out:main.efi main.o