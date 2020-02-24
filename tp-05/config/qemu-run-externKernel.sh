#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA="-drive file=/home/aider/vm/pnl-tp.img,format=raw"
HDB="-drive file=/home/aider/vm/myHome.img,format=raw"
KERNEL=linux-4.19/arch/x86/boot/bzImage

# Linux kernel options
CMDLINE="root=/dev/sda1 rw console=ttyS0 kgdboc=ttyS1 kgdbwait"

FLAGS="--enable-kvm -s"

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     -net user -net nic \
     -serial mon:stdio \
     -serial tcp::1234,server,nowait\
     -boot c -m 2G \
     -kernel "${KERNEL}" -append "${CMDLINE}"

