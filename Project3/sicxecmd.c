#include "20171630.h"

int assemble(char* filename) {
	FILE* fp = fopen(filename, "r");
	FILE* lst_fp;
	FILE* obj_fp;

	//Filename is divided into name and extension part and it stores.
	char *lst;
	char *obj;
	char asm_input[255];
	char tok[4][10];
	char temp[30];
	int len;
	int i, j, k, type;
	len = strlen(filename);
	lst = (char*)malloc(sizeof(char)*len);
	obj = (char*)malloc(sizeof(char)*len);
	

	//Verify if it exists
	if (fp == NULL) {
		printf("%s file does not exist.\n", filename);
		return FALSE;
	}

	//.asm extension confirmation
	if (strcmp(filename+len-4, ".asm") != 0) {
		printf("%s is not an .asm file.\n", filename);
		fclose(fp);
		return FALSE;
	}
	strcpy(lst, filename);
	lst[len - 3] = 'l';
	lst[len - 2] = 's';
	lst[len - 1] = 't';
	strcpy(obj, filename);
	obj[len - 3] = 'o';
	obj[len - 2] = 'b';
	obj[len - 1] = 'j';



	if (pass_1(filename) == FALSE) {
		FreeAll_s(tmp_symhead);
		fclose(fp);
		return FALSE;
	}
	else if (pass_2(filename) == FALSE) {
		FreeAll_s(tmp_symhead);
		fclose(fp);
		return FALSE;
	}
	else {
		FreeAll_s(symhead);
		symhead = (SYMTAB*)malloc(sizeof(SYMTAB));
		symhead->next = tmp_symhead->next;
	}

	//make lst file
	lst_fp = fopen(lst, "w");
	for (i = 1; i <= cnt; i++) {
		fgets(asm_input, 255, fp);

		//tok initialization
		tok[0][0] = '\0';
		tok[1][0] = '\0';
		tok[2][0] = '\0';
		tok[3][0] = '\0';
		temp[0] = '\0';

		sscanf(asm_input, "%s %s %s %s", tok[0], tok[1], tok[2], tok[3]);
		//line number
		fprintf(lst_fp, "%-5d", i * 5);
		if (line[i] == -2) {
			len = strlen(asm_input);
			if (asm_input[len - 1] == '\n') {
				asm_input[len - 1] = '\0';
			}
			fprintf(lst_fp, "\t\t%-30s", asm_input);
		}
		//base, end
		else if (line[i] == -1 || i == cnt) {
			fprintf(lst_fp, "\t\t\t%s\t%-13s\t",tok[0],tok[1]);
		}
		else {
			if (tok[2][0] == '\0') {
				fprintf(lst_fp, "\t%04X\t\t%s\t%-13s\t",line[i], tok[0], tok[1]);
			}
			else if (tok[3][0] == '\0') {
				if (tok[1][strlen(tok[1]) - 1] == ',') {
					strcpy(temp, tok[1]);
					strcat(temp, " ");
					strcat(temp, tok[2]);
					fprintf(lst_fp, "\t%04X\t\t%s\t%-13s\t", line[i], tok[0], temp);
				}
				else {
					fprintf(lst_fp, "\t%04X\t%s\t%s\t%-13s\t",line[i], tok[0], tok[1], tok[2]);
				}
			}
			else {
				strcpy(temp, tok[2]);
				strcat(temp, " ");
				strcat(temp, tok[3]);
				fprintf(lst_fp, "\t%04X\t%s\t%s\t%-13s\t", line[i], tok[0], tok[1], temp);
			}
		}

		type = objcode[i].type;
		//base, resw, resb
		if (type < 0) {
			fprintf(lst_fp, "\n");
			continue;
		}
		j = 0;
		while (j < type) {
			fprintf(lst_fp, "%02X", objcode[i].objectcode[j]);
			j++;
		}
		fprintf(lst_fp, "\n");
	}
	fclose(lst_fp);
	fclose(fp);

	//make obj file
	obj_fp = fopen(obj, "w");

	//Header Record
	fprintf(obj_fp, "H");
	fprintf(obj_fp, "%-6s", file_name);
	fprintf(obj_fp, "%06X%06X\n", line[1], line[cnt] - line[1]);

	//Text Record
	j = 1;
	while (1) {
		while (j <= cnt) {
			if (objcode[j].type < 0 || line[j] < 0) {
				j++;
			}
			else break;
		}
		if (j > cnt) break;

		fprintf(obj_fp, "T%06X", line[j]);
		len = 0;
		i = j;
		while (i <= cnt) {
			if (objcode[i].type == -2) {
				i++;
				break;
			}
			else if (objcode[i].type == -1) {
				i++;
				continue;
			}
			if (len + objcode[i].type > 30) break;
			len += objcode[i].type;
			i++;
		}
		fprintf(obj_fp, "%02X", len);
		for (len = j; len < i; len++) {
			if (objcode[len].type < 0) continue;
			for (k = 0; k < objcode[len].type; k++) {
				fprintf(obj_fp, "%02X", objcode[len].objectcode[k]);
			}
		}
		j = i;
		fprintf(obj_fp, "\n");
	}

	//Modification Record
	for (i = 1; i < cnt; i++) {
		if (objcode[i].type == 4 && objcode[i].flag != 10)
			fprintf(obj_fp, "M%06X%02X\n", line[i] + 1, 5);
	}

	//End Record
	for (i = 1; i < cnt; i++) {
		if (objcode[i].type > 0) {
			fprintf(obj_fp, "E%06X\n", line[i]); break;
		}
	}

	fclose(obj_fp);
	printf("[%s], [%s]\n", lst, obj);
	free(objcode);
	free(line);
	return TRUE;
}

int pass_1(char* filename) {
	FILE* fp = fopen(filename, "r");

	char asm_input[255];
	char tok[3][10];
	int loc = 0;
	int end_flag = FALSE;
	int i, type;

	line = (int*)malloc(sizeof(int) * 10000000);
	cnt = 0;

	tmp_symhead = (SYMTAB*)malloc(sizeof(SYMTAB));
	tmp_symhead->next = NULL;

	while (fgets(asm_input, 255, fp) != NULL) {
		cnt++;

		//loc range check
		if (loc > 0xFFFF) {
			printf("ERROR: Line [%d].\n", cnt * 5);
			return FALSE;
		}

		//tok initialization
		tok[0][0] = '\0';
		tok[1][0] = '\0';
		tok[2][0] = '\0';

		sscanf(asm_input, "%s %s %s", tok[0], tok[1], tok[2]);

		//START
		if (cnt == 1) {
			if (strcmp(tok[1], "START") == 0) {
				strcpy(file_name, tok[0]);
				if (tok[2] != '\0')
					loc = strtol(tok[2], NULL, 16);
				else loc = 0;
			}
			//If you do not start with START
			else {
				printf("ERROR: 'START' dosen't exist.\n");
				fclose(fp);
				return FALSE;
			}
			line[cnt] = loc;
		}
		//END
		else if (strcmp(tok[0], "END") == 0) {
			line[cnt] = loc;
			end_flag = TRUE;
			break;
		}
		//comment
		else if (tok[0][0] == '.') {
			line[cnt] = -2;
			continue;
		}
		//BASE
		else if (strcmp(tok[0], "BASE") == 0) {
			line[cnt] = -1;
			continue;
		}
		else {
			line[cnt] = loc;
			//label existence
			if (asm_input[0] != ' ' && asm_input[0] != '\t') {
				//Label and Loc Storage on Symbol Table
				if (!(insertSymbol(tok[0], loc))) {//If the length over or duplicated label or first letter is not alphabetical
					printf("ERROR: Line [%d].\n", cnt * 5);
					fclose(fp);
					return FALSE;
				}
				//Pull the token one by one
				strcpy(tok[0], tok[1]);
				strcpy(tok[1], tok[2]);
			}

			//BYTE
			if (strcmp(tok[0], "BYTE") == 0) {
				if (tok[1][0] == 'X' && tok[1][1] == '\'' && tok[1][(int)strlen(tok[1]) - 1] == '\'') {
					if (((int)strlen(tok[1]) - 3) % 2 != 0) {//even numbered hexadecimal check
						printf("ERROR: Line [%d].\n", cnt * 5);
						fclose(fp);
						return FALSE;
					}

					for (i = 2; i < (int)strlen(tok[1]) - 1; i++) {
						if (isxdigit(tok[1][i]) == FALSE) {
							printf("ERROR: Line [%d].\n", cnt * 5);
							fclose(fp);
							return FALSE;
						}
					}
					loc += (((int)strlen(tok[1]) - 3) / 2);
				}
				else if (tok[1][0] == 'C' && tok[1][1] == '\'' && tok[1][(int)strlen(tok[1]) - 1] == '\'') {
					for (i = 2; i < (int)strlen(tok[1]) - 1; i++) {
						if (tok[1][i]<'A' || tok[1][i]>'Z') {
							printf("ERROR: Line [%d].\n", cnt * 5);
							fclose(fp);
							return FALSE;
						}
					}
					loc += ((int)strlen(tok[1]) - 3);
				}
				else {
					printf("ERROR: Line [%d].\n", cnt * 5);
					fclose(fp);
					return FALSE;
				}
			}
			//WORD
			else if (strcmp(tok[0], "WORD") == 0) {
				for (i = 0; i < (int)strlen(tok[1]); i++) {
					if (tok[1][i]<'0' || tok[1][i]>'9') {
						printf("ERROR: Line [%d].\n", cnt * 5);
						fclose(fp);
						return FALSE;
					}
				}
				loc += 3;
			}
			//RESB
			else if (strcmp(tok[0], "RESB") == 0) {
				for (i = 0; i < (int)strlen(tok[1]); i++) {
					if (tok[1][i]<'0' || tok[1][i]>'9') {
						printf("ERROR: Line [%d].\n", cnt * 5);
						fclose(fp);
						return FALSE;
					}
				}
				loc += atoi(tok[1]);
			}
			//RESW
			else if (strcmp(tok[0], "RESW") == 0) {
				for (i = 0; i < (int)strlen(tok[1]); i++) {
					if (tok[1][i]<'0' || tok[1][i]>'9') {
						printf("ERROR: Line [%d].\n", cnt * 5);
						fclose(fp);
						return FALSE;
					}
				}
				loc += atoi(tok[1]) * 3;
			}
			//instruction
			else {
				type = search_type(tok[0]);
				if (type < 0) {
					printf("ERROR: Line [%d].\n", cnt * 5);
					fclose(fp);
					return FALSE;
				}
				loc += type;
			}
		}
	}
	if (end_flag == FALSE) {
		printf("ERROR: 'END' dosen't exist.\n");
		fclose(fp);
		return FALSE;
	}
	fclose(fp);
	return TRUE;
}

int pass_2(char* filename) {
	FILE* fp = fopen(filename, "r");

	char asm_input[255];
	char tok[4][10];
	int pc = 0, base = 0;
	int i, j;
	int temp;
	int temp2;
	int opcode, type;
	OpMode op;

	objcode = (ObjectCode*)malloc(sizeof(ObjectCode)*(cnt + 1));

	for (i = 1; i <= cnt; i++) {

		//tok initialization
		tok[0][0] = '\0';
		tok[1][0] = '\0';
		tok[2][0] = '\0';
		tok[3][0] = '\0';

		fgets(asm_input, 255, fp);

		pc = line[i + 1];
		for (j = i + 2; pc < 0; j++) {
			pc = line[j];
		}

		if (asm_input[0] == '.') {
			objcode[i].type = -1;
			continue;
		}

		//When Label was there
		if (asm_input[0] != ' ' && asm_input[0] != '\t') {
			sscanf(asm_input, "%s %s %s %s", tok[3], tok[0], tok[1], tok[2]);
		}
		//in the absence of label
		else {
			sscanf(asm_input, "%s %s %s", tok[0], tok[1], tok[2]);
		}
		//START
		if (strcmp(tok[0], "START") == 0) {
			objcode[i].type = 0;
		}
		//END
		else if (strcmp(tok[0], "END") == 0) {
			objcode[i].type = -1;
			break;
		}
		//BASE
		else if (strcmp(tok[0], "BASE") == 0) {
			objcode[i].type = -1;
			base = searchSymbol(tok[1]);
			if (base == -1) {
				printf("ERROR: Line [%d].\n", i * 5);
				fclose(fp);
				return FALSE;
			}
		}
		//RESW or RESB
		else if (strcmp(tok[0], "RESW") == 0 || strcmp(tok[0], "RESB") == 0) {
			objcode[i].type = -2;
		}
		//WORD
		else if (strcmp(tok[0], "WORD") == 0) {
			temp = atoi(tok[1]);
			objcode[i].objectcode[0] = (temp >> 16) & 255;
			objcode[i].objectcode[1] = (temp >> 8) & 255;
			objcode[i].objectcode[2] = (temp) & 255;
			objcode[i].type = 3;

		}
		//BYTE
		else if (strcmp(tok[0], "BYTE") == 0) {
			if (tok[1][0] == 'X') {
				int hex = 1;
				j = 0;
				while (j++ < (int)strlen(tok[1]) - 5) {
					hex *= 16;
				}
				sscanf(tok[1], "X'%X'", &temp);
				j = 0;
				while (j*2 < (int)strlen(tok[1]) - 3) {
					objcode[i].objectcode[j++] = temp/hex;
					hex /= 16 * 16;
				}
				objcode[i].type = ((int)strlen(tok[1]) - 3) / 2;
			}
			else if (tok[1][0] == 'C') {
				j = 2;
				temp = 0;
				while (j < (int)strlen(tok[1]) - 1) {
					objcode[i].objectcode[temp++] = tok[1][j++];
				}
				objcode[i].type = (int)strlen(tok[1]) - 3;
			}
		}
		//instruction
		else {
			type = search_type(tok[0]);
			opcode = search_opcode(tok[0]);
			//type 1
			if (type == 1) {
				if (tok[1][0] != '\0') {
					printf("ERROR: Line [%d].\n", i * 5);
					fclose(fp);
					return FALSE;
				}
				objcode[i].objectcode[0] = opcode;
				objcode[i].type = type;
			}
			//type 2
			else if (type == 2) {
				objcode[i].objectcode[0] = opcode;
				objcode[i].type = type;
				if (tok[1][0] == '\0') {//1 token except for label
					printf("ERROR: Line [%d].\n", i * 5);
					fclose(fp);
					return FALSE;
				}
				else if (tok[2][0] == '\0') {//2 tokens except for label
					temp = findreg(tok[1]);
					if (temp == -1) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					objcode[i].objectcode[1] = (temp << 4);
				}
				else if (tok[3][0] == '\0') {//3 tokens except for label, if there are registers
					if (tok[1][(int)strlen(tok[1]) - 1] != ',') {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					tok[1][(int)strlen(tok[1]) - 1] = '\0';
					temp = findreg(tok[1]);
					temp2 = findreg(tok[2]);
					if (temp == -1 || temp2 == -1) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					objcode[i].objectcode[1] = (temp << 4) + temp2;
				}
				else {
					printf("ERROR: Line [%d].\n", i * 5);
					fclose(fp);
					return FALSE;
				}
			}
			//type 3
			else if (type == 3) {
				objcode[i].objectcode[0] = opcode;
				objcode[i].type = type;
				if (tok[1][0] == '\0') {//1 token except for label
					objcode[i].objectcode[0] += 3;
					objcode[i].objectcode[1] = 0;
					objcode[i].objectcode[2] = 0;
					objcode[i].objectcode[3] = 0;
				}
				else if (tok[2][0] == '\0') {//2 tokens except for label
					op = mode(tok[1]);
					if (op.mode == -1 || op.mode == 0) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					else if (op.mode == 4) {
						if (op.address > 0xFFF) {
							printf("ERROR: Line [%d].\n", i * 5);
							fclose(fp);
							return FALSE;
						}
						objcode[i].objectcode[0] += 1;
						objcode[i].objectcode[1] = op.address / (16 * 16);
						objcode[i].objectcode[2] = op.address % (16 * 16);
						objcode[i].flag = 10;
					}
					else {
						objcode[i].objectcode[0] += op.mode;
						if (op.address >= pc - 2048 && op.address <= pc + 2047) {
							temp = op.address - pc;
							objcode[i].objectcode[1] = (1 << 5) + (15 & (temp >> 8));
							objcode[i].objectcode[2] = (255 & temp);
						}
						else if (op.address >= base && op.address <= base + 4095) {
							temp = op.address - base;
							objcode[i].objectcode[1] = (1 << 6) + (15 & (temp / 256));
							objcode[i].objectcode[2] = (temp % 256);
						}
						else {
							printf("ERROR: Line [%d].\n", i * 5);
							fclose(fp);
							return FALSE;
						}
					}
				}
				else if (tok[3][0] == '\0') {//3 tokens except for label, if there are registers
					if (tok[1][(int)strlen(tok[1]) - 1] != ',') {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					else {
						tok[1][(int)strlen(tok[1]) - 1] = '\0';
					}
					op = mode(tok[1]);
					if (strcmp(tok[2], "X") != 0) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					if (op.mode == -1 || op.mode == 0) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					else if (op.mode == 4) {
						if (op.address > 0xFFF) {
							printf("ERROR: Line [%d].\n", i * 5);
							fclose(fp);
							return FALSE;
						}
						objcode[i].objectcode[0] += 1;
						objcode[i].objectcode[1] = (1 << 7) + op.address / (16 * 16);
						objcode[i].objectcode[2] = op.address % (16 * 16);
					}
					else {
						objcode[i].objectcode[0] += op.mode;
						if (op.address >= pc - 2048 && op.address <= pc + 2047) {
							temp = op.address - pc;
							objcode[i].objectcode[1] = (1 << 5) + (1 << 7) + (15 & (temp >> 8));
							objcode[i].objectcode[2] = (255 & temp);
						}
						else if (op.address >= base && op.address <= base + 4095) {
							temp = op.address - base;
							objcode[i].objectcode[1] = (1 << 6) + (1 << 7) + (temp / 256);
							objcode[i].objectcode[2] = (255 & temp);
						}
						else {
							printf("ERROR: Line [%d].\n", i * 5);
							fclose(fp);
							return FALSE;
						}
					}
				}
				else {
					printf("ERROR: Line [%d].\n", i * 5);
					fclose(fp);
					return FALSE;
				}
			}
			//type 4
			else if (type == 4) {
				objcode[i].objectcode[0] = opcode;
				objcode[i].type = type;
				if (tok[1][0] == '\0') {//1 token except for label
					objcode[i].objectcode[0] += 3;
					objcode[i].objectcode[1] = 0;
					objcode[i].objectcode[2] = 0;
					objcode[i].objectcode[3] = 0;
				}
				else if (tok[2][0] == '\0') {//2 tokens except for label
					op = mode(tok[1]);
					if (op.mode == -1 || op.mode == 0) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					else if (op.mode == 4) {
						objcode[i].objectcode[0] += 1;
						objcode[i].objectcode[1] = (1 << 4) + op.address / (16 * 16 * 16 * 16);
						objcode[i].objectcode[2] = 255 & (op.address >> 8);
						objcode[i].objectcode[3] = 255 & (op.address);
						objcode[i].flag = 10;
					}
					else {
						objcode[i].objectcode[0] += op.mode;
						objcode[i].objectcode[1] = (1 << 4) + (15 & (op.address >> 16));
						objcode[i].objectcode[2] = 255 & (op.address >> 8);
						objcode[i].objectcode[3] = 255 & (op.address);
					}
				}
				else if (tok[3][0] == '\0') {//3 tokens except for label, if there are registers
					if (tok[1][(int)strlen(tok[1]) - 1] != ',') {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					else {
						tok[1][(int)strlen(tok[1]) - 1] = '\0';
					}
					op = mode(tok[1]);
					if (strcmp(tok[2], "X") != 0) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					if (op.mode == -1 || op.mode == 0) {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
					else if (op.mode == 3) {
						objcode[i].objectcode[0] += op.mode;
						objcode[i].objectcode[1] = (1 << 4) + (1 << 7) + (15 & (op.address >> 16));
						objcode[i].objectcode[2] = 255 & (op.address >> 8);
						objcode[i].objectcode[3] = 255 & (op.address);
					}
					else {
						printf("ERROR: Line [%d].\n", i * 5);
						fclose(fp);
						return FALSE;
					}
				}
				else {
					printf("ERROR: Line [%d].\n", i * 5);
					fclose(fp);
					return FALSE;
				}
			}
		}
	}
	fclose(fp);
	return TRUE;
}

int searchSymbol(char* label) {
	SYMTAB* cur = tmp_symhead->next;
	while (cur != NULL) {
		if (strcmp(label, cur->label) == 0) {
			return cur->locctr;
		}
		cur = cur->next;
	}
	return -1;
}

int insertSymbol(char* label, int loc) {
	SYMTAB* node = (SYMTAB*)malloc(sizeof(SYMTAB));

	if ((int)strlen(label) > 6)
		return FALSE;
	if (label[0]<'A' || label[0] > 'Z')
		return FALSE;

	node->next = NULL;
	node->locctr = loc;
	strcpy(node->label, label);

	SYMTAB* cur = tmp_symhead->next;

	//When there's no symbol yet
	if (cur == NULL) {
		tmp_symhead->next = node;
		return TRUE;
	}

	//When the new symbol was first placed
	if (strcmp(node->label, cur->label) < 0) {
		node->next = tmp_symhead->next;
		tmp_symhead->next = node;
		return TRUE;
	}
	else {
		while (TRUE) {
			//If the label is the same, -1 return
			if (strcmp(node->label, cur->label) == 0) {
				return FALSE;
			}
			//end
			else if (cur->next == NULL) {
				cur->next = node;
				break;
			}
			//middle
			else if (strcmp(node->label, (cur->next)->label) < 0) {
				node->next = cur->next;
				cur->next = node;
				break;
			}
			cur = cur->next;
		}
	}
	return TRUE;
}

int ShowSymbol() {
	SYMTAB* cur = symhead->next;
	//if SYMTAB is empty
	if (cur == NULL) {
		printf("No SYMTAB existing! Assemble first.\n");
		return FALSE;
	}

	else {
		while (cur != NULL) {
			printf("        %s\t%04X\n", cur->label, cur->locctr);
			cur = cur->next;
		}
		return TRUE;
	}
}

void FreeAll_s(SYMTAB* root) {
	SYMTAB* node;
	while (root != NULL) {
		node = root;
		root = root->next;
		free(node);
	}
}

int search_type(char* mnemonic) {
	int hash;
	int flag = 0;
	if (mnemonic[0] == '+') {
		flag = 1;
		sscanf(mnemonic, "+%s", mnemonic);
	}
	hash = hashFunction(mnemonic);
	Opcode_table* cur = hashTable[hash]->next;
	while (cur != NULL) {
		if (!strcmp(mnemonic, cur->instruction)) {
			if (strcmp(cur->type, "1") == 0) {
				if (flag == 1)
					return -1;
				return 1;
			}
			else if (strcmp(cur->type, "2") == 0) {
				if (flag == 1)
					return -1;
				return 2;
			}
			else if (strcmp(cur->type, "3/4") == 0 && flag == 0) return 3;
			else if (strcmp(cur->type, "3/4") == 0 && flag == 1) return 4;
		}
		else cur = cur->next;
	}
	return -1;
}

int findreg(char* str) {
	if (strcmp(str, "A") == 0)return 0;
	else if (strcmp(str, "X") == 0)return 1;
	else if (strcmp(str, "L") == 0)return 2;
	else if (strcmp(str, "PC") == 0)return 8;
	else if (strcmp(str, "SW") == 0)return 9;
	else if (strcmp(str, "B") == 0)return 3;
	else if (strcmp(str, "S") == 0)return 4;
	else if (strcmp(str, "T") == 0)return 5;
	else if (strcmp(str, "F") == 0)return 6;
	else return -1;
}

OpMode mode(char* str) {
	OpMode op;
	int addr;
	int temp;

	op.mode = 0;
	op.address = 0;
	addr = findreg(str);
	if (addr != -1) {
		op.mode = 0;
		op.address = addr;
		return op;
	}
	else if (str[0] == '#') {
		op.mode = 1;
		sscanf(str, "#%s", str);
	}
	else if (str[0] == '@') {
		op.mode = 2;
		sscanf(str, "@%s", str);
	}
	else {
		op.mode = 3;
	}

	addr = searchSymbol(str);
	if (addr == -1) {
		if (op.mode == 1) {
			temp = atoi(str);
			if (temp < 0 || temp>0xfffff) {
				op.mode = -1;
				return op;
			}
			op.mode = 4;
			op.address = temp;
			return op;
		}
		op.mode = -1;
		return op;
	}
	else op.address = addr;
	return op;
}
