#!/bin/bash
BZIMAGE=linux/arch/x86/boot/bzImage
ROOTFS=bookworm.img

qemu-system-x86_64     -m 16G     -smp 8,sockets=1,cores=8     -net nic,model=e1000e  -net user,host=10.0.2.25,hostfwd=tcp::10022-:22     -enable-kvm     -snapshot     -append "root=/dev/sda console=ttyS0 earlyprintk=serial net.ifnames=0"     -drive format=raw,file="$ROOTFS"     -kernel "$BZIMAGE"

# This is the command to ssh to the qemu machine
# ssh -i bookworm.id_rsa -p 10022 root@localhost

