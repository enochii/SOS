# How to add apps to SOS

- write a *.c, for example, saolei.c
- compile and link
  ```shell
  gcc -I ../include/ -c -fno-builtin -Wall -o saolei.o saolei.c -fno-stack-protector
  nasm -I ../include -f elf -o start.o start.asm
  ld --Ttext 0x1000 -o saolei saolei.o start.o ../lib/orangescrt.a
  ```
- tar all apps in inst.tar
  ```shell
  tar vcf inst.tar kernel.bin echo pwd saolei
  ```
- into disk
  ```shell
  #count our inst.tar
  ls -l inst.tar | awk -F " " '{print $5}'
  # for example, the above result is 143360
  dd if=inst.tar of=../80m.img seek=27131392 bs=1 count=143360 conv=notrunc
  ```