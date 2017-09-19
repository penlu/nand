all: nand nandf

nand: nand.c
	gcc --std=c11 -Ofast -march=native nand.c -o nand

nandf: nandf.c
	gcc --std=c11 -Ofast -march=native nandf.c -o nandf
