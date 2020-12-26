PW = $(shell cat ~/文档/PW)
# Kernel variable ===============================================================================
K_SRC_C = $(wildcard ./Kernel/*.c)
K_SRC_I = $(wildcard ./Kernel/g_reg_only/*.c)
K_SRC_H = Kernel/head.S Kernel/AP_Boot.S
K_OBJ_C = $(patsubst %.c,%.o,$(K_SRC_C)) 
K_OBJ_H = $(patsubst %.S,%.o,$(K_SRC_H))
K_PRC_H = $(patsubst %.S,%.s,$(K_SRC_H))
K_OBJ_I = $(patsubst %.c,%.o,$(K_SRC_I))
K_SYMS  = kallsyms/kallsyms
# User thread variable ==========================================================================
U_SRC = $(wildcard ./user/*.c)			# Need to make sure init.c is the first entry
U_OBJ = $(patsubst %.c,%.o,$(U_SRC))
# Test thread variable ==========================================================================
T_SRC = $(wildcard ./test/*.c)			# Need to make sure init.c is the first entry
T_OBJ = $(patsubst %.c,%.o,$(T_SRC))
# UEFI variable =================================================================================
EDKDIR = ./edk2
OVMF_DSC = OvmfPkg/OvmfPkgX64.dsc
TRGOBJ = Kernel.bin test.bin user.bin
BFLAGS = -a X64 -t GCC5
# Dump info variable ============================================================================
DUMPOBJ = system user.sys test.sys
DUMPRST = $(patsubst %,%.s,$(DUMPOBJ))
# QEMU variable =================================================================================
QFLAGS = -machine q35 -cpu EPYC -accel kvm -smp 4,cores=2,threads=2,sockets=1 -m 1G -device nvme,drive=D22,serial=1234 -serial stdio -net none
# Local variable ================================================================================
run:CFLAGS = -mcmodel=large -fno-builtin -fno-stack-protector -m64
run:LFLAGS = -b elf64-x86-64
run:QMEUFL = $(QFLAGS)
dbg:CFLAGS = -ggdb3 -mcmodel=large -fno-builtin -fno-stack-protector -m64
dbg:LFLAGS = -ggdb3 -b elf64-x86-64
dbg:QMEUFL = -s -S $(QFLAGS)

# Kernel compile ================================================================================
$(K_OBJ_C):%.o:%.c
	gcc $(CFLAGS) -c $< -o $@ 
	
$(K_OBJ_I):%.o:%.c
	gcc $(CFLAGS) -mgeneral-regs-only -c $< -o $@

$(K_PRC_H):%.s:%.S
	gcc -E $< > $@

$(K_OBJ_H):%.o:%.s
	as --64 -o $@ $<

system_0: $(K_OBJ_H) $(K_OBJ_C) $(K_OBJ_I)
	ld $(LFLAGS) -z muldefs -o $@ $^ -T ./Kernel/Kernel.lds

$(K_SYMS): $(K_SYMS).c
	gcc -o $@ $<

$(K_SYMS).S: system_0 $(K_SYMS)
	nm -n $(word 1,$^) | $(word 2,$^) > $@

$(K_SYMS).o: $(K_SYMS).S
	gcc -c $< -o $@

system: $(K_OBJ_H) $(K_OBJ_C) $(K_OBJ_I) $(K_SYMS).o
	ld $(LFLAGS) -z muldefs -o $@ $^ -T ./Kernel/Kernel.lds

Kernel.bin: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $^ $@

# User compile ==================================================================================
$(U_OBJ):%.o:%.c
	gcc $(CFLAGS) -c $< -o $@

user.sys: $(U_OBJ)
	ld $(LFLAGS) -z muldefs -o $@ $^ -T user/user.ids

user.bin: user.sys
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $^ $@
# Test compile ==================================================================================
$(T_OBJ):%.o:%.c
	gcc $(CFLAGS) -c $< -o $@

test.sys: $(T_OBJ)
	ld $(LFLAGS) -z muldefs -o $@ $^ -T test/test.ids

test.bin: test.sys
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $^ $@
# UEFI compile ==================================================================================
# UEFI enviroment need to be set up each time:
# 1, cd edk2
# 2, source edksetup.sh BaseTools
build_base:
	make -C ./edk2/BaseTools
$(EDKDIR)/Build/My/DEBUG_GCC5/X64/BootX64.efi: MyPkg/BootX64/BootX64.c MyPkg/BootX64/BootX64.h MyPkg/BootX64/BootX64.inf MyPkg/MyPkg.dec MyPkg/MyPkg.dsc
	-rm -rf $(EDKDIR)/MyPkg
	cp -rf MyPkg $(EDKDIR)
	cd $(EDKDIR) && build $(BFLAGS) -p $(word 5,$^) -m $(word 3,$^)
$(EDKDIR)/Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd:
	cd $(EDKDIR) && build $(BFLAGS) -p $(OVMF_DSC) 

# Make disk =====================================================================================
hda.disk: $(EDKDIR)/Build/My/DEBUG_GCC5/X64/BootX64.efi $(TRGOBJ)
	-rm hda.disk
	qemu-img create hda.disk 1G
	echo -e "g\nn\n\n\n+250M\nt\n1\nn\n\n\n\nw" | fdisk hda.disk
	echo $(PW) | sudo -S losetup --offset 1048576 --sizelimit 262144000 /dev/loop0 hda.disk
	sudo mkfs.vfat -F 32 /dev/loop0 -I
	sudo losetup -d /dev/loop0
	sudo losetup --offset 263192576 --sizelimit 810532352 /dev/loop0 hda.disk
	sudo mkfs.vfat /dev/loop0 -I
	sudo losetup -d /dev/loop0
	# Prevent mount before delete complete, sleep 0.2s
	sleep 0.2
	sudo mount hda.disk mnt -t vfat -o loop,offset=1048576
	sudo mkdir mnt/EFI/
	sudo mkdir mnt/EFI/Boot
	sudo cp $(word 1,$^) mnt/EFI/Boot
	sleep 0.5
	sudo umount mnt
	sudo mount hda.disk mnt -t vfat -o loop,offset=263192576
	sudo mkdir mnt/abc
	sudo cp test.txt mnt/abc
	sudo cp $(TRGOBJ) KEYBOARD.DEV mnt
	sleep 0.5
	sudo umount mnt

nvme.disk: 
	-rm nvme.disk
	qemu-img create nvme.disk 1G

run: hda.disk $(EDKDIR)/Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd nvme.disk
	qemu-system-x86_64 $(QMEUFL) -hda $(word 1,$^) -bios $(word 2,$^) -drive file=$(word 3,$^),if=none,id=D22

dbg: hda.disk $(EDKDIR)/Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd nvme.disk
	# start: gdb
	# connecting: target remote: 1234
	qemu-system-x86_64 $(QMEUFL) -hda $(word 1,$^) -bios $(word 2,$^) -drive file=$(word 3,$^),if=none,id=D22
	
clean:
	-rm $(K_OBJ_C) $(K_OBJ_I) $(K_PRC_H) $(K_OBJ_H) $(U_OBJ) $(T_OBJ)
	-rm $(K_SYMS) $(K_SYMS).S $(K_SYMS).o
	-rm system system_0 user.sys test.sys
	-rm Kernel.bin user.bin test.bin
	-rm *.disk
	-rm $(DUMPRST)
	-rm -rf $(EDKDIR)/MyPkg

tests:
	@echo $(DUMPRST)

dump: $(DUMPOBJ)
	objdump -D $(word 1,$^) > $(word 1,$^).s
	objdump -D $(word 2,$^) > $(word 2,$^).s
	objdump -D $(word 3,$^) > $(word 3,$^).s

# GitHub ========================================================================================
sub_pull:
	git submodule update --init --recursive
sub_update:
	git submodule foreach --recursive 'git pull origin master'
commit: clean
	git add -A
	@echo "Please type in commit comment: "; \
	read comment; \
	git commit -m"$$comment"
sync: commit 
	git push -u origin master