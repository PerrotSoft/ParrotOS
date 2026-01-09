#!/usr/bin/env bash
set -e
SDK_DIR="$(dirname "$0")/../edk2"
HW_DIR="$(dirname "$0")"
DSC_FILE="$HW_DIR/Minimal.dsc"
BUILD_FILE="$HW_DIR/build_number.txt"
OUTPUT_DIR="$HW_DIR/out"
EFI_SOURCE="$SDK_DIR/Build/DEBUG_GCC5/X64/ParrotOS.efi"

mkdir -p "$OUTPUT_DIR"
echo "--- Сборка UEFI приложения ---"
cd "$SDK_DIR"
source edksetup.sh
build -a X64 -t GCC5 -p "$DSC_FILE"
cd "$HW_DIR"

if [ ! -f "$EFI_SOURCE" ]; then
    echo "Ошибка: EFI файл не найден по пути $EFI_SOURCE"
    exit 1
fi

echo "--- Создание структуры папок ---"
USB_ROOT="$OUTPUT_DIR/usb_root"
mkdir -p "$USB_ROOT/EFI/BOOT"
cp "$EFI_SOURCE" "$USB_ROOT/EFI/BOOT/BOOTX64.EFI"

echo "--- Генерация образа диска (boot.img) ---"
IMG_FILE="$OUTPUT_DIR/boot.img"
dd if=/dev/zero of="$IMG_FILE" bs=1M count=64
mkfs.vfat "$IMG_FILE"
mmd -i "$IMG_FILE" ::/EFI
mmd -i "$IMG_FILE" ::/EFI/BOOT
mcopy -i "$IMG_FILE" "$EFI_SOURCE" ::/EFI/BOOT/BOOTX64.EFI

echo "--- Генерация ISO образа ---"
ISO_FILE="$OUTPUT_DIR/boot.iso"
mkisofs -U -A "MyUEFI" -V "UEFI_BOOT" -J -joliet-long -r -v \
    -eltorito-alt-boot -e EFI/BOOT/BOOTX64.EFI -no-emul-boot \
    -o "$ISO_FILE" "$USB_ROOT"

qemu-system-x86_64 -hda "$IMG_FILE" -m 256M -bios /usr/share/ovmf/OVMF.fd -net none

echo "------------------------------------------------"
read -p "Хотите записать проект на флешку? (y/n): " flash_yn
if [[ $flash_yn == [Yy]* ]]; then
    lsblk 
    echo "ВНИМАНИЕ: Все данные на выбранном диске будут УДАЛЕНЫ!"
    read -p "Введите имя устройства (например, sdb или sdc): " dev_name
    
    if [ -b "/dev/$dev_name" ]; then
        read -p "Записать как образ (IMG) или просто скопировать файлы (файлы)? (img/file): " mode
        if [ "$mode" == "img" ]; then
            sudo dd if="$IMG_FILE" of="/dev/$dev_name" bs=4M status=progress
            sync
        else
            echo "Для копирования файлов флешка должна быть отформатирована в FAT32."
            read -p "Введите путь к точке монтирования флешки (например /media/user/FLASH): " mount_path
            mkdir -p "$mount_path/EFI/BOOT"
            cp -r "$USB_ROOT/"* "$mount_path/"
            sync
        fi
        echo "Готово!"
    else
        echo "Устройство /dev/$dev_name не найдено."
    fi
fi
echo "--- Работа с Git ---"
read -p "Скомпилировать и сохранить в Git? (y/n): " git_yn
if [[ $git_yn == [Yy]* ]]; then
    if [ ! -f "$BUILD_FILE" ]; then echo 0 > "$BUILD_FILE"; fi
    BUILD_NUM=$(($(cat "$BUILD_FILE") + 1))
    echo $BUILD_NUM > "$BUILD_FILE"

    git add .
    read -p "Использовать авто-коммит? (y/n): " auto_git
    if [[ $auto_git == [Yy]* ]]; then
        git commit -m "ParrotOS Build $BUILD_NUM"
    else
        read -p "Введите комментарий: " comment
        git commit -m "$comment (Build $BUILD_NUM)"
    fi
    git push
fi