#! /bin/bash

# Fixer les variables avec les chemins de vos fichiers
HDA="-drive file=/home/aider/vm/pnl-tp.img,format=raw"
HDB="-drive file=/home/aider/vm/myHome.img,format=raw"
FLAGS="--enable-kvm "

exec qemu-system-x86_64 ${FLAGS} \
     ${HDA} ${HDB} \
     -net user -net nic \
     -boot c -m 2G
