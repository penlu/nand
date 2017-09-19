all: nand nandf

nand: nand.c
	gcc --std=c11 -Ofast -funroll-loops -march=native nand.c -o nand

nandf: nandf.c
	gcc --std=c11 -Ofast -funroll-loops -march=native nandf.c -o nandf
