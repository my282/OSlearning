#!/bin/bash

# --- 設定 ---
EDK2_PATH="$HOME/src/edk2"
DSC_FILE="OvmfPkg/OvmfPkgX64.dsc"
TARGET_ARCH="X64"
TOOLCHAIN="GCC" 
BUILD_MODE="RELEASE" # または DEBUG
ORIGINAL_DIR=&(pwd)
# コピー元のEFIファイル名（あなたのinfのBASE_NAMEに合わせてください）
EFI_NAME="memorymap.efi"

# --- ビルド実行 ---
# --- 成果物の回収 ---
ORIGINAL_DIR=$(pwd)
LOG_FILE="$ORIGINAL_DIR/build.log"

cd $EDK2_PATH
source edksetup.sh

echo "Building... (Log: $LOG_FILE)"

# ログファイルを絶対パスで指定して保存
build -p $DSC_FILE -a $TARGET_ARCH -t $TOOLCHAIN -b $BUILD_MODE > "$LOG_FILE" 2>&1

# エラーチェック
if [ $? -ne 0 ]; then
    echo "Build failed!"
    # 失敗したときだけログを表示
    cat "$LOG_FILE"
    exit 1
fi
# 成果物が埋もれている深いパス
BUILD_DIR="$EDK2_PATH/Build/OvmfX64/${BUILD_MODE}_${TOOLCHAIN}/${TARGET_ARCH}"
# ※注意: ここはあなたのパッケージ構成によって微妙に変わるかも
EFI_SOURCE=$(find $BUILD_DIR -name "$EFI_NAME" | head -n 1)

if [ -z "$EFI_SOURCE" ]; then
    echo "Error: $EFI_NAME not found in build output."
    exit 1
fi

# コピー先（プロジェクト直下のdistディレクトリ）
PROJECT_ROOT="$HOME/OSlearning"
DIST_DIR="$PROJECT_ROOT/dist/EFI/BOOT"

mkdir -p $DIST_DIR
cp "$EFI_SOURCE" "$DIST_DIR/BOOTX64.EFI"

echo "----------------------------------------"
echo "Success! File copied to:"
echo "$DIST_DIR/BOOTX64.EFI"
echo "----------------------------------------"

# 1.200MBのファイル生成
qemu-img create -f raw "$ORIGINAL_DIR/disk.img" 200M
# 2.作成したファイルをFAT32形式でフォーマット
mkfs.fat -n 'FamOS' -s 2 -f 2 -R 32 -F 32 "$ORIGINAL_DIR/disk.img" 
# 3.必要なフォルダを作る
mmd -i "$ORIGINAL_DIR/disk.img" ::/EFI
mmd -i "$ORIGINAL_DIR/disk.img" ::/EFI/BOOT
# 4. コンパイルした.efiファイルをdisk.imgの中にコピー
mcopy -i "$ORIGINAL_DIR/disk.img" "$DIST_DIR/BOOTX64.EFI" ::/EFI/BOOT/BOOTX64.EFI

echo "disk.img created successfully."
