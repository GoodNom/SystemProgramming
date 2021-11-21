#include "20171630.h"

int loader(char* objfile, int file_num) {
	int len;
	len = strlen(objfile);

	if (strcmp(objfile + len - 4, ".obj") != 0) {
		printf("ERROR: %s is not an .obj file.\n", objfile);
		return FALSE;
	}

	if (linking_loader_pass1(objfile, file_num))
		return TRUE;
	else
		return FALSE;
}

int linking_loader_pass1(char* objfile, int file_num) {
	int i;
	char obj_line[255];
	char record_type;
	char cs_name[7];
	int cs_addr, cs_len;
	unsigned int d_index = 1;
	char d_sym[7];
	int d_addr;

	FILE* fp = fopen(objfile, "r");

	if (fp == NULL) {
		printf("ERROR: %s file does not exist.\n", objfile);
		return FALSE;
	}


	//Header record
	fgets(obj_line, 255, fp);
	if (obj_line[0] != 'H') {
		printf("ERROR: There is no header record in this object file.\n");
		fclose(fp);
		return FALSE;
	}
	sscanf(obj_line, "%c%6s%06X%06X", &record_type, cs_name, &cs_addr, &cs_len);

	//search the control section name from the table
	for (i = 0; i < file_num; i++) {
		if (!strcmp(ESTAB[i].cs_name, cs_name)) {
			printf("ERROR: Control section with this name already exists.\n");
			fclose(fp);
			return FALSE;
		}
	}

	CSLTH = cs_len;
	//add ESTAB
	strcpy(ESTAB[file_num].cs_name, cs_name);
	ESTAB[file_num].addr = CSADDR;
	ESTAB[file_num].len = CSLTH;

	while (fgets(obj_line, 255, fp) != NULL) {
		//End record
		if (obj_line[0] == 'E')break;

		//Define record
		if (obj_line[0] == 'D') {
			d_index = 1;
			while (d_index < strlen(obj_line) - 1) {
				sscanf(obj_line + d_index, "%6s%06X", d_sym, &d_addr);

				if (search_sym(file_num, d_sym)) {
					printf("ERROR: Symbol name already exists.\n");
					fclose(fp);
					return FALSE;
				}

				insert_estab_sym(file_num, d_sym, d_addr + CSADDR);
				d_index += 12;
			}
		}
	}
	CSADDR += CSLTH;
	fclose(fp);
	return TRUE;
}

int linking_loader_pass2(char* objfile, int file_num, int total_file_num) {
	char obj_line[255];
	char record_type;
	char cs_name[7];
	int cs_addr, cs_len;
	//for Text record
	int t_len, t_start_addr, t_addr, t_index = 1;
	int obj_byte;
	//for Refer record
	int r_num, r_addr;
	unsigned int r_index = 1;
	char r_sym[7];
	//for Modification record
	int m_addr, m_len, m_ref, m_code, m_tmp;
	char m_code_txt[10];
	int save_value;
	//+,-
	char operator;

	FILE* fp = fopen(objfile, "r");

	if (fp == NULL) {
		printf("ERROR: %s file does not exist.\n", objfile);
		return FALSE;
	}

	typedef struct REFER {
		char sym_name[7];
		int sym_addr;
	}refer;
	refer ref_list[6];

	EXECADDR = PROGADDR;
	CSADDR = ESTAB[file_num].addr;

	//Header record
	fgets(obj_line, 255, fp);
	sscanf(obj_line, "%c%6s%06X%06X", &record_type, cs_name, &cs_addr, &cs_len);
	CSLTH = cs_len;

	while (fgets(obj_line, 255, fp) != NULL) {
		//End record
		if (obj_line[0] == 'E') {
			if (isHex(obj_line + 1))
				EXECADDR = CSADDR + strtol(obj_line + 1, NULL, 16);
			break;
		}

		//Refer record
		else if (obj_line[0] == 'R') {
			r_index = 1;
			ref_list[1].sym_addr = CSADDR;

			while (r_index < strlen(obj_line) - 1) {
				sscanf(obj_line + r_index, "%02X%6s", &r_num, r_sym);
				r_addr = search_sym_addr(r_sym, total_file_num);

				if (r_addr == FALSE) {
					printf("ERROR: Symbol not found in ESTAB.\n");
					fclose(fp);
					return FALSE;
				}

				ref_list[r_num].sym_addr = r_addr;
				strcpy(ref_list[r_num].sym_name, r_sym);
				r_index += 8;
			}
		}

		//Text record
		else if (obj_line[0] == 'T') {
			sscanf(obj_line, "%c%06X%02X", &record_type, &t_start_addr, &t_len);

			//Length of object code in this record in bytes
			t_len *= 2;
			//'T'(1)+ start_address(6) + length of object code(2) = 9
			t_len += 9;
			//object code starts from index 9 in T record
			t_index = 9;
			//set location to CSADDR + specified address
			t_addr = t_start_addr + CSADDR;

			//move object code from record to location
			while (t_index < t_len) {
				sscanf(obj_line + t_index, "%02X", &obj_byte);
				mem[t_addr] = obj_byte;
				t_addr++;
				t_index += 2;
			}

		}

		//Modification record
		else if (obj_line[0] == 'M') {
			sscanf(obj_line, "%c%06X%02X%c%02X", &record_type, &m_addr, &m_len, &operator,&m_ref);
			m_addr += CSADDR;

			//calculate code to modify
			sprintf(m_code_txt, "%02X%02X%02X", mem[m_addr], mem[m_addr + 1], mem[m_addr + 2]);
			sscanf(m_code_txt, "%06X", &m_code);
			save_value = m_code / 0x100000;

			//add or subtract symbol value at location
			if (operator=='+') {
				m_code += ref_list[m_ref].sym_addr;
			}
			else if (operator=='-') {
				m_code -= ref_list[m_ref].sym_addr;
			}
			else {
				printf("ERROR: operator needs to be '+' or '-'.\n");
				fclose(fp);
				return FALSE;
			}

			sprintf(m_code_txt, "%08X", m_code);

			if (m_len == 0x05) {
				sscanf(m_code_txt, "%03X%1X%02X%02X", &m_tmp, &mem[m_addr], &mem[m_addr + 1], &mem[m_addr + 2]);
				mem[m_addr] += save_value * 0x10;
			}
			else {
				sscanf(m_code_txt, "%02X%02X%02X%02X", &m_tmp, &mem[m_addr], &mem[m_addr + 1], &mem[m_addr + 2]);
			}
		}
	}
	CSADDR += CSLTH;
	fclose(fp);
	return TRUE;
}

void show_loadmap(int file_num) {
	int i;
	//for calculating total length of the program (sum of each section's length)
	int total_len = 0;

	ES_TAB cur;
	ESTAB_sym* cur_sym;

	printf("control\tsymbol\taddress\tlength\n");
	printf("section\tname\n");
	printf("----------------------------------\n");

	for (i = 0; i < file_num; i++) {
		cur = ESTAB[i];
		//first print out current control section's data
		printf("%s\t\t%04X\t%04X\n", cur.cs_name, cur.addr, cur.len);
		//add current control section's length
		total_len += cur.len;
		//set pointer to current contrl section's symbol list
		cur_sym = cur.es_sym->next;
		while (cur_sym != NULL) {
			//print out current control section's symbol list
			printf("\t%6s\t%04X\n", cur_sym->symbol, cur_sym->address);
			cur_sym = cur_sym->next;
		}
	}
	printf("----------------------------------\n");
	//print out total length of the whole program
	printf("\t  total length  %04X\n", total_len);
}


void insert_estab_sym(int file_num, char* sym_name, int sym_addr) {
	//create new node for ESTAB symbol
	ESTAB_sym* node = (ESTAB_sym*)malloc(sizeof(ESTAB_sym));
	node->next = NULL;
	node->address = sym_addr;
	strcpy(node->symbol, sym_name);

	//set cursor to current control section's node's es_sym
	ESTAB_sym* cur = ESTAB[file_num].es_sym->next;

	//if there is no symbol in ESTAB for this control section, insert node
	if (cur == NULL) ESTAB[file_num].es_sym->next = node;

	//else find the end of the list and insert node
	else {
		while (cur->next != NULL) cur = cur->next;
		cur->next = node;
	}
}

int search_sym_addr(char* sym_name, int file_num) {
	int i;
	ESTAB_sym* cur;

	//search for symbol and return symbol's address
	for (i = 0; i < file_num; i++) {
		cur = ESTAB[i].es_sym->next;
		while (cur != NULL) {
			if (!strcmp(cur->symbol, sym_name))
				return cur->address;
			cur = cur->next;
		}
	}

	//if symbol not found in ESTAB
	return FALSE;
}

int search_sym(int file_num, char* sym_name) {
	//symbol list of current control section
	ESTAB_sym* cur = ESTAB[file_num].es_sym->next;

	while (cur != NULL) {
		if (!strcmp(cur->symbol, sym_name)) return TRUE;
		cur = cur->next;
	}

	return FALSE;
}

void save_bp(int bp_loc) {
	bp* cur = bp_head->next;

	//new node
	bp* node = (bp*)malloc(sizeof(bp));
	node->loc = bp_loc;
	node->next = NULL;

	//insert at the end of list
	if (cur == NULL) bp_head->next = node;
	else {
		while (cur->next != NULL) {
			cur = cur->next;
		}
		cur->next = node;
	}

	printf("\t\t[ok] create breakpoint %X\n", bp_loc);
}

void clear_bp() {
	bp* cur = bp_head->next;

	if (cur == NULL) {
		printf("\t\tno breakpoints existing!\n");
		return;
	}

	//free nodes in bp list except for the head node
	while (cur != NULL) {
		bp* next = cur->next;
		free(cur);
		cur = next;
	}

	bp_head->next = NULL;

	printf("\t\t[ok] clear all breakpoints\n");
}

void print_bp() {
	bp* cur = bp_head->next;

	printf("\t\tbreakpoint\n");
	printf("\t\t----------\n");

	while (cur != NULL) {
		printf("\t\t%X\n", cur->loc);
		cur = cur->next;
	}
}

void run() {
	//n,i,x,b,p
	int n = 0, i = 0, x = 0, b = 0, p = 0;
	//save total length
	int run_len = ESTAB[0].len;
	//register values, A=0, X=1, L=2, B=3, S=4, T=5, PC=8, SW=9
	static int reg[10];
	//set start address
	static int cur_addr = 0;
	//code length
	int code_len;
	//first half byte
	int halfbyte;
	//one full object code
	int objcode;
	//displacementlacement, 12 bit (for format 3)
	int displacement;
	//address, 20 bit (for format 4)
	int addr;
	//for register instructions
	int reg1, reg2;
	//for target address
	int target_addr = 0;
	//for immediate addressing mode
	int imm;
	//set opcode
	int opcode;
	//when debugging, do not stop at the same bp as last time (it starts from there)
	static int last_bp = -1;

	//if there was no bp before, set L = endpoint, PC = PROGADDR
	if (!in_debug) {
		reg[2] = PROGADDR + run_len;
		reg[8] = cur_addr = EXECADDR;
	}

	//run through file until bp
	while (1) {
		//if file reaches the last code
		if (reg[8] == PROGADDR + run_len) {
			//print register values
			print_reg(reg);

			//initialize register values
			for (i = 0; i < 10; i++) reg[i] = 0;

			printf("\t    End Program\n");

			//set debug flag to 0 to show no debugging is going on
			in_debug = 0;

			return;
		}

		//if file reaches bp
		if (search_bp(reg[8]) && last_bp != reg[8]) {
			//print register values
			print_reg(reg);

			printf("\t    Stop at checkpoint[%X]\n", reg[8]);

			last_bp = reg[8];

			//set debug flag to 1 to show debugging is going on
			in_debug = 1;

			return;
		}

		//set current address to PC
		cur_addr = reg[8];

		//check first half byte of the code
		halfbyte = mem[cur_addr] / 0x10;

		//first half byte 9~B:format 2, else format 3/4 (no format 1 in this ex)
		//if format 2, 2 bytes
		if (halfbyte >= 0x9 && halfbyte <= 0xB) code_len = 2;
		//if format 3 or 4, 3 or 4 bytes
		else code_len = 3;

		//set opcode, get rid of n & i
		opcode = mem[cur_addr] & 0xFC;
		//printf("opcode: %X\n",opcode);

		//if format 2
		if (code_len == 2) {
			//PC += 2
			reg[8] += 2;

			//if CLEAR, r1<-0
			if (mem[cur_addr] == 0xB4) {
				reg1 = mem[cur_addr + 1] / 0x10;
				reg[reg1] = 0;
			}

			//if COMPR, (r1):(r2)
			else if (mem[cur_addr] == 0xA0) {
				//A0(r1)(r2)
				reg1 = mem[cur_addr + 1] / 0x10;
				reg2 = mem[cur_addr + 1] % 0x10;

				if (reg1 < 0 || reg1>9 || reg2 < 0 || reg2>9) {
					printf("ERROR: register number is incorrect.\n");
					return;
				}

				if (reg[reg1] < reg[reg2]) reg[9] = -1;
				else if (reg[reg1] == reg[reg2]) reg[9] = 0;
				else reg[9] = 1;
			}

			//if TIXR, X<-X+1, X:r1
			else if (mem[cur_addr] == 0xB8) {
				//X<-X+1
				reg[1]++;
				reg1 = mem[cur_addr + 1] / 0x10;

				if (reg1 < 0 || reg1>9) {
					printf("ERROR: register number is incorrect.\n");
					return;
				}

				//X:r1
				if (reg[1] < reg[reg1]) reg[9] = -1;
				else if (reg[1] == reg[reg1]) reg[9] = 0;
				else reg[9] = 1;
			}

			else {
				printf("ERROR: invalid OPCODE in format 2.\n");
				return;
			}

			continue;
		}//end of format 2

		//check if format 4
		else {
			halfbyte = mem[cur_addr + 1] / 0x10;
			//if 3rd half byte of object code is odd, e=1, format 4.
			if (halfbyte % 2) code_len = 4;
		}

		//if format 3
		if (code_len == 3) {
			//PC += 3
			reg[8] += 3;

			//set object code
			objcode = mem[cur_addr] * 0x10000 + mem[cur_addr + 1] * 0x100 + mem[cur_addr + 2];

			//set nixbp (since format 3, e=0)
			n = objcode & 0x020000;
			i = objcode & 0x010000;
			x = objcode & 0x008000;
			b = objcode & 0x004000;
			p = objcode & 0x002000;

			displacement = objcode & 0x000FFF;

			//check sign bit(leftmost bit) of displacement
			//if minus, set left bits to F
			if (displacement & 0x000800) displacement = displacement | 0xFFFFF000;

			//if simple addressing mode, n=1, i=1
			if (n && i) {
				//if pc relative, displacement + PC
				if (p) target_addr = reg[8] + displacement;
				//if base relative, displacement + B
				else if (b) target_addr = displacement + reg[3];
				//if direct addressing mode
				else if (!p && !b) target_addr = displacement + PROGADDR;
				//if X 
				if (x) target_addr += reg[1];
				imm = mem[target_addr] * 0x10000 + mem[target_addr + 1] * 0x100 + mem[target_addr + 2];
			}

			//if indirect addressing mode, n=1, i=0
			else if (n && !i) {
				if (p) displacement += reg[8];
				else if (b) displacement += reg[3];
				else if (!p && !b) displacement += PROGADDR;
				if (x) displacement += reg[1];

				//since this is indirect addressing mode,
				//target address = object code at updated displacement
				target_addr = mem[displacement] * 0x10000 + mem[displacement + 1] * 0x100 + mem[displacement + 2];

				imm = mem[target_addr] * 0x10000 + mem[target_addr + 1] * 0x100 + mem[target_addr + 2];
			}

			//if immediate addressing mode, n=0, i=1
			else {
				//no need to set target addr
				imm = displacement;
				if (p) imm += reg[8];
				else if (b) imm += reg[3];
			}


			run_form3or4(opcode, imm, code_len, target_addr, reg, x);
		}//end of format 3

		//if format 4
		else if (code_len == 4) {
			//for(int k=0;k<100;k++) printf("INFORM4");
			reg[8] += 4;

			//set object code
			objcode = mem[cur_addr] * 0x1000000 + mem[cur_addr + 1] * 0x10000 + mem[cur_addr + 2] * 0x100 + mem[cur_addr + 3];


			//set n,i,x,b,p,(e=1)
			n = objcode & 0x02000000;
			i = objcode & 0x01000000;
			x = objcode & 0x00800000;
			b = objcode & 0x00400000;
			p = objcode & 0x00200000;

			//set address to rightmost 5 half bytes
			addr = objcode & 0x000FFFFF;

			if (addr & 0x00080000) addr = addr | 0xFFF00000;

			//if simple addressing mode, n=1, i=1
			if (n&&i) {
				if (p) target_addr = addr + reg[8];
				else if (b) target_addr = addr + reg[2];
				else if (!p && !b) target_addr = addr + PROGADDR;
				if (x) target_addr += reg[1];

				imm = mem[target_addr] * 0x1000000 + mem[target_addr + 1] * 0x10000 + mem[target_addr + 2] * 0x100 + mem[target_addr + 3];
			}

			//if indirect mode, n = 1, i = 0
			else if (n && !i) {
				if (p) addr += reg[8];
				else if (b) addr += reg[3];
				if (x) addr += reg[1];

				target_addr = mem[target_addr] * 0x1000000 + mem[target_addr + 1] * 0x10000 + mem[target_addr + 2] * 0x100 + mem[target_addr + 3];

				imm = mem[target_addr] * 0x1000000 + mem[target_addr + 1] * 0x10000 + mem[target_addr + 2] * 0x100 + mem[target_addr + 3];
			}

			//if immediate addressing mode, n=0, i=1
			else {
				imm = addr;
				if (p) imm += reg[8];
				else if (b) imm += reg[3];
			}

			run_form3or4(opcode, imm, code_len, target_addr, reg, x);

		}//end of format 4

		//print_reg(reg);
		//return;
	}//end of run before bp

	return;
}

int search_bp(int bp_loc) {
	bp* cur = bp_head->next;

	while (cur != NULL) {
		if (cur->loc == bp_loc) return 1;
		cur = cur->next;
	}

	return 0;
}

void run_form3or4(int opcode, int imm, int code_len, int target_addr, int* reg, int x) {
	//LDA A<-m..m+2
	if (opcode == 0x00) {
		reg[0] = imm;
	}

	//LDB B<-m..m+2
	else if (opcode == 0x68) {
		reg[3] = imm;
	}

	//LDT T<-m..m+2
	else if (opcode == 0x74) {
		reg[5] = imm;
	}

	//LDCH A[rightmost byte] <- m
	else if (opcode == 0x50) {
		//erase rightmost byte
		reg[0] = reg[0] & 0xFFFF00;

		//put memory value in rightmost byte
		//if x, A<-m[x]
		if (x) reg[0] += imm / 0x10000;
		else reg[0] += imm & 0x0000FF;
	}

	//STA m..m+2<-A
	else if (opcode == 0x0C) {
		store_mem(reg[0], target_addr, code_len * 2);
	}

	//STX m..m+2<-X
	else if (opcode == 0x10) {
		store_mem(reg[1], target_addr, code_len * 2);
	}

	//STL m..m+2<-L
	else if (opcode == 0x14) {
		store_mem(reg[2], target_addr, code_len * 2);
	}

	//STCH m<-A[rightmost byte]
	else if (opcode == 0x54) {
		store_mem(reg[0] & 0x000000FF, target_addr, code_len * 2);
	}

	//J PC<-m
	else if (opcode == 0x3C) {
		reg[8] = target_addr;
	}

	//JSUB L<-PC; PC<-m
	else if (opcode == 0x48) {
		reg[2] = reg[8];
		reg[8] = target_addr;
		//for(int i=0;i<100;i++) printf("%06X\n",target_addr);
	}

	//JLT PC<-m if CC set to < (CC saved in SW reg)
	else if (opcode == 0x38) {
		if (reg[9] < 0) reg[8] = target_addr;
	}

	//JEQ PC<-m if CC set to =
	else if (opcode == 0x30) {
		if (reg[9] == 0) reg[8] = target_addr;
	}

	//RSUB PC<-L
	else if (opcode == 0x4C) reg[8] = reg[2];

	//COMP A:m..m+2
	else if (opcode == 0x28) {
		if (reg[0] < imm) reg[9] = -1;
		else if (reg[0] == imm) reg[9] = 0;
		else reg[9] = 1;
	}

	//TD: continue after setting CC to <
	else if (opcode == 0xE0) reg[9] = -1;

	//RD: continue after setting CC to =
	else if (opcode == 0xD8) reg[9] = 0;

	//WD: skip
}

void store_mem(int value, int address, int halfbyte) {
	char txt_value[10];

	//if format 4, store 4 bytes starting from addr
	if (halfbyte == 8) {
		value = value & 0xFFFFFFFF;
		sprintf(txt_value, "%08X", value);
		sscanf(txt_value, "%02X%02X%02X%02X", &mem[address], &mem[address + 1], &mem[address + 2], &mem[address + 3]);
	}

	//if format 3, store 3 bytes starting from addr
	else {// halfbyte == 6
		//make sure there are only 3 bytes
		value = value & 0x00FFFFFF;
		sprintf(txt_value, "%06X", value);
		sscanf(txt_value, "%02X%02X%02X", &mem[address], &mem[address + 1], &mem[address + 2]);
	}
}

void print_reg(int* reg) {
	printf("A : %06X  X : %06X\n", reg[0], reg[1]);
	printf("L : %06X PC : %06X\n", reg[2], reg[8]);
	printf("B : %06X  S : %06X\n", reg[3], reg[4]);
	printf("T : %06X\n", reg[5]);
}