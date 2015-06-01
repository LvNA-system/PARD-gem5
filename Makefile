run:
	LINUX_IMAGE=/opt/m5-system/prm/disks/PARDg5GM.img M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt $(FLAGS) configs/PARDg5-V.py --num-cpus=4 --mem-size=12GB --cpu-clock=100MHz --kernel=linux-2.6.28.4-prm/vmlinux

restore:
	M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt $(FLAGS) configs/PARDg5-V.py --num-cpus=4 --mem-size=12GB --cpu-clock=100MHz -r 1

debug:
	LINUX_IMAGE=/opt/m5-system/prm/disks/PARDg5GM.img M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so gdb --args ../gem5-offical/build/X86/gem5.opt $(FLAGS) configs/PARDg5-V.py --num-cpus=4 --mem-size=12GB --cpu-clock=100MHz --kernel=linux-2.6.28.4-prm/vmlinux

timing:
	LINUX_IMAGE=/opt/m5-system/prm/disks/PARDg5GM.img M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so $(EXT) ../gem5-offical/build/X86/gem5.opt $(FLAGS) configs/PARDg5-V.py --num-cpus=4 --mem-size=8GB --cpu-type=timing

debug-timing:
	M5_EXTLIB=./build/PARDX86/libpardg5v_debug.so gdb --args ../gem5-offical/build/X86/gem5.debug configs/example/xfs.py --cpu-type=timing  --caches --l2cache


memtest:
	M5_EXTLIB=build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt configs/example/fs.py --cpu-type=timing --kernel=x86_64-vmlinux-2.6.28.4 --caches --l2cache

socat:
	sudo socat TCP:localhost:3500 TUN:192.168.0.100/24,up,tun-type=tap

prm:
	M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt configs/PRM.py
