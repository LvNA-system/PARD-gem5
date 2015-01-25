run:
	M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt configs/PARDg5-V.py --num-cpus=4 --mem-size=12GB --cpu-clock=100MHz

debug:
	M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so gdb --args ../gem5-offical/build/X86/gem5.opt $(FLAGS) configs/PARDg5-V.py --num-cpus=4 --mem-size=12GB --cpu-clock=100MHz

timing:
	M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so $(EXT) ../gem5-offical/build/X86/gem5.opt configs/PARDg5-V.py --num-cpus=4 --mem-size=12GB --cpu-type=timing --cpu-clock=100MHz

debug-timing:
	M5_EXTLIB=./build/PARDX86/libpardg5v_debug.so gdb --args ../gem5-offical/build/X86/gem5.debug configs/example/xfs.py --cpu-type=timing  --caches --l2cache


memtest:
	M5_EXTLIB=build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt configs/example/fs.py --cpu-type=timing --kernel=x86_64-vmlinux-2.6.28.4 --caches --l2cache

socat:
	sudo socat TCP:localhost:3500 TUN:192.168.0.100/24,up,tun-type=tap

prm:
	M5_EXTLIB=./build/PARDX86/libpardg5v_opt.so ../gem5-offical/build/X86/gem5.opt configs/PRM.py
