#include "20171630.h"

void print_mem(int start, int end) {
	int i, j;

	//print memory address
	printf("%04X0 ", start / 16);

	//print with space before starting address
	if (start % 16 != 0) {
		for (i = 0; i < start % 16; i++) printf("   ");
	}

	//print middle column
	for (i = start; i <= end; i++) {
		printf("%02X ", mem[i]);
		if (i % 16 == 15) {
			printf("; ");
			for (j = i - 15; j <= i; j++) {
				//if the value is not in range 20~7E or if it's before the start address, print '.'
				if (mem[j] < 0x20 || mem[j]>0x7E || j < start) printf(".");
				else printf("%c", mem[j]);
			}
			printf("\n");
			//print memory address
			if (i < end) printf("%04X0 ", (i + 1) / 16);
		}
	}

	//if the end address did not reach the rightmost address
	if (end % 16 != 15) {
		//print with space after ending address
		for (i = end % 16 + 1; i < 16; i++) printf("   ");
		printf("; ");

		//the rightmost column
		for (j = end / 0x10 * 0x10; j <= end / 0x10 * 0x10 + 15; j++) {
			//if the value is not in range 20~7E or if the address is not in range, print '.'
			if (mem[j] < 0x20 || mem[j]>0x7E || j < start || j > end) printf(".");
			else printf("%c", mem[j]);
		}
		printf("\n");
	}
}

//print 10 lines
void dump() {
	if (end_index == 0xFFFFF) {
		print_mem(0, 159);
		end_index = 159;
	}
	//if out of boundary, print only to 0xFFFFF
	else if (end_index + 160 > 0xFFFFF) {
		print_mem(end_index + 1, 0xFFFFF);
		end_index = 0xFFFFF;
	}
	else {
		print_mem(end_index + 1, end_index + 160);
		end_index += 160;
	}
}

//print 10 lines from start
void dumpstart(int start) {
	if (start + 159 > 0xFFFFF) print_mem(start, 0xFFFFF);
	else print_mem(start, start + 159);
}
//print from start to finish
void dumpstartend(int start, int end) {
	print_mem(start, end);
}

void editvalue(int address, int value) {
	mem[address] = value;
}

void fillvalue(int start, int end, int value) {
	int i;
	for (i = start; i <= end; i++) {
		mem[i] = value;
	}
}

void reset_mem() {
	memset(mem, 0, sizeof(mem));
}
