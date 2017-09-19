all: nand nandf

nand: nand.c
	gcc --std=c11 -O3 nand.c -o nand

nandf: nandf.c
	gcc --std=c11 -O3 nandf.c -o nandf
