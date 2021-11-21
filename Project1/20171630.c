#include "20171630.h"

int main() {
	int i;
	//number of arguments(ex. "dump 14, 37": size = 3)
	int size = 0;
	//flag to check for invalid input
	int flag = FALSE;
	//to get the input
	char* input;
	//to save tokenized input
	char* modified_input[100];
	//for storing input for history
	char* save_input;
	char* temp;

	//hash value by hash function
	int hash_value;

	//result opcode
	int res_opcode;

	end_index = 0xFFFFF;
	memset(mem, 0, MEM_SIZE);

	//initializing head node for history
	head = (Node*)malloc(sizeof(Node));
	head->next = NULL;

	//initializing hashTable for opcode
	hashTable = (Opcode_table**)malloc(sizeof(Opcode_table*)*HASH_TABLE_SIZE);
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		hashTable[i] = (Opcode_table*)malloc(sizeof(Opcode_table));
		hashTable[i]->next = NULL;
	}

	//importing opcode.txt
	FILE* fp;
	char op_input[255];
	char* modified_op_input[3];
	fp = fopen("opcode.txt", "r");
	if (fp != NULL) {

		//read line
		while (fgets(op_input, 255, fp) != NULL) {

			//tokenize
			modified_op_input[0] = strtok(op_input, " \n\t");
			modified_op_input[1] = strtok(NULL, " \n\t");
			modified_op_input[2] = strtok(NULL, " \n\t");

			//save opcode information
			op_mem = (Opcode_table*)malloc(sizeof(Opcode_table));
			op_mem->opcode = strtol(modified_op_input[0], NULL, 16);
			strcpy(op_mem->instruction, modified_op_input[1]);
			strcpy(op_mem->type, modified_op_input[2]);

			//hash function
			hash_value = ((int)op_mem->instruction[0]+strlen(op_mem->instruction))% 20;

			//hash setting
			op_mem->next = hashTable[hash_value]->next;
			hashTable[hash_value]->next = op_mem;
		}
	}

	//file open failure
	else {
		printf("ERROR: The ""opcode.txt"" file does not exist.\n");
		return 0;
	}
	fclose(fp);


	while (1) {
		flag = FALSE;
		input = (char*)malloc(sizeof(char) * 100);
		save_input = (char*)malloc(sizeof(char) * 100);
		temp = (char*)malloc(sizeof(char) * 100);

		printf("sicsim> ");

		fgets(input, 100, stdin);
		if(input[strlen(input)-1] != '\n')
			while(getchar() != '\n');

		if (strlen(input) > 90){ 
			printf("ERROR: The input is too long.\n");
			continue;
		}
		if (input[0] == '\n') continue;

		//separating tokens based on space
		size = 0;
		modified_input[0] = strtok(input, " \n\t");
		while (modified_input[size] != NULL) {
			size++;
			modified_input[size] = strtok(NULL, " \n\t");

			//if too many arguments
			if (size > 6) {
				printf("ERROR: Too many arguments.\n");
				flag = TRUE;
				break;
			}
		}
		if (flag) continue;

		//check arguments not separated by commas
		for (i = 1; i < size - 1; i++) {
			if (modified_input[i][strlen(modified_input[i]) - 1] != ',' && modified_input[i + 1][0] != ',') {
				printf("ERROR: Invalid input.\n");
				flag = TRUE;
				break;
			}
		}
		if (flag) continue;

		//for checking ','
		i = 1;
		temp[0] = '\0';
		while (modified_input[i] != NULL) {
			strcat(temp, modified_input[i]);
			i++;
		}
		if (temp[0] == ',') {
			printf("ERROR: Invalid input.\n");
			flag = TRUE;
			continue;
		}
		for (i = 1; i < (int)strlen(temp); i++) {
			if (temp[i] == ',') {
				if (temp[i + 1] == ',' || temp[i + 1] == '\0') {
					printf("ERROR: Invalid input.\n");
					flag = TRUE;
					break;
				}
			}
		}
		if (flag) continue;

		//refine input
		size = 1;
		modified_input[1] = strtok(temp, ",");
		while (modified_input[size] != NULL) {
			size++;
			modified_input[size] = strtok(NULL, ",");

			//if too many arguments
			if (size > 4) {
				printf("ERROR: Too many arguments.\n");
				flag = TRUE;
				break;
			}
		}
		if (flag) continue;

		//save input in correct format
		save_input[0] = '\0';
		for (i = 0; i < size; i++) {
			strcat(save_input, modified_input[i]);
			if (i == 0 && size > 1) strcat(save_input, " ");
			if (i >= 1 && size > i + 1) {
				strcat(save_input, ", ");
			}
		}

		if (size == 1) {
			//if q[uit]
			if (!strcmp("quit", modified_input[0]) || !strcmp("q", modified_input[0])) {
				FreeAll(head);
				destructor();
				free(input);
				free(save_input);
				free(temp);
				break;
			}
			//if h[elp]
			else if (!strcmp("help", modified_input[0]) || !strcmp("h", modified_input[0])) {
				printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp][start, end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\n");
				insertNode(head, save_input);
				continue;
			}
			//if d[ir]
			else if (!strcmp("dir", modified_input[0]) || !strcmp("d", modified_input[0])) {
				printdir();
				insertNode(head, save_input);
				continue;
			}
			//if hi[story]
			else if (!strcmp("history", modified_input[0]) || !strcmp("hi", modified_input[0])) {
				insertNode(head, save_input);
				ShowAll(head);
				continue;
			}
		}

		//if du[mp] [start, end]
		if (!strcmp("dump", modified_input[0]) || !strcmp("du", modified_input[0])) {
			int start, end;

			switch (size) {
			case 1:
				dump();
				insertNode(head, save_input);
				break;
			case 2:
				//check if the address is hexadecimal
				if (isHex(modified_input[1]) == FALSE) {
					printf("ERROR: Address not in hexadecimal format.\n");
					flag = TRUE;
					break;
				}

				//change the starting address from char to hex
				start = strtol(modified_input[1], NULL, 16);

				//boundary check
				if (start > 0xFFFFF || start < 0) {
					printf("ERROR: The starting address is out of range.\n");
					flag = TRUE;
					break;
				}

				dumpstart(start);
				insertNode(head, save_input);
				break;
			case 3:
				//check if the address is hexadecimal
				if (isHex(modified_input[1]) == FALSE || isHex(modified_input[2]) == FALSE) {
					printf("ERROR: Address not in hex form.\n");
					flag = TRUE;
					break;
				}

				//change start and end address from char to hex
				start = strtol(modified_input[1], NULL, 16);
				end = strtol(modified_input[2], NULL, 16);

				//boundary check
				if (start < 0 || start > 0xFFFFF || end < 0 || end > 0xFFFFF) {
					printf("ERROR: Address out of range.\n");
					flag = TRUE;
					break;
				}
				if (start > end) {
					printf("ERROR: Range error.\n");
					flag = TRUE;
					break;
				}

				dumpstartend(start, end);
				insertNode(head, save_input);
				break;
			default:
				printf("ERROR: Too many arguments.\n");
				flag = TRUE;
				break;
			}
		}

		//if e[dit] address, value
		else if (!strcmp("edit", modified_input[0]) || !strcmp("e", modified_input[0])) {
			if (size == 3) {

				//check if the address is hexadecimal
				if (isHex(modified_input[1]) == FALSE || isHex(modified_input[2]) == FALSE) {
					printf("ERROR: Address not in hex form.\n");
					flag = TRUE;
					continue;
				}

				//change address and value from char to hex
				int address = strtol(modified_input[1], NULL, 16);
				int value = strtol(modified_input[2], NULL, 16);

				//boundary check
				if (address < 0 || address > 0xFFFFF) {
					printf("ERROR: Address out of range.\n");
					flag = TRUE;
					continue;
				}
				else if (value < 0x00 || value > 0xFF) {
					printf("ERROR: Value out of range.\n");
					flag = TRUE;
					continue;
				}
				else {
					editvalue(address, value);
					insertNode(head, save_input);
					continue;
				}
			}
			else {
				printf("ERROR: Input has to be e[dit] address, value. See h[elp].\n");
				flag = TRUE;
				continue;
			}
		}

		//if f[ill] start, end, value
		else if (!strcmp("f", modified_input[0]) || !strcmp("fill", modified_input[0])) {
			if (size == 4) {
				//check if the address is hexadecimal
				if (isHex(modified_input[1]) == FALSE || isHex(modified_input[2]) == FALSE || isHex(modified_input[3]) == FALSE) {
					printf("ERROR: Address or Value not in hex form.\n");
					flag = TRUE;
					continue;
				}

				//change start, end address and value from char to hex
				int start = strtol(modified_input[1], NULL, 16);
				int end = strtol(modified_input[2], NULL, 16);
				int value = strtol(modified_input[3], NULL, 16);

				//boundary check
				if (start < 0 || start>0xFFFFF || end < 0 || end>0xFFFFF || start > end) {
					printf("ERROR: Address out of range.\n");
					flag = TRUE;
					continue;
				}
				else if (value < 0x00 || value>0xFF) {
					printf("ERROR: Value out of range.\n");
					flag = TRUE;
					continue;
				}
				else {
					fillvalue(start, end, value);
					insertNode(head, save_input);
					continue;
				}
			}
			else {
				printf("ERROR: Four arguments needed. See h[elp].\n");
				flag = TRUE;
				continue;
			}

		}

		//if reset
		else if (!strcmp("reset", modified_input[0]) && size == 1) {
			reset_mem();
			insertNode(head, save_input);
			continue;
		}

		//if opcode mnemonic
		else if (!strcmp("opcode", modified_input[0]) && size == 2) {

			for (i = 0; i < (int)strlen(modified_input[1]); i++) {
				if (modified_input[1][i]<'A' || modified_input[1][i] > 'Z') {
					printf("ERROR: Enter mnemonic in uppercase.\n");
					flag = TRUE;
					break;
				}
			}
			if (flag) continue;

			//hash function
			hash_value = ((int)modified_input[1][0]+strlen(modified_input[1])) % 20;

			//search opcode
			res_opcode = search_opcode(hash_value, modified_input[1]);

			if (res_opcode != -1) {
				printf("opcode is %X\n", res_opcode);
				insertNode(head, save_input);
				continue;
			}
			else {
				printf("ERROR: Cannot find opcode. See opcodelist.\n");
				flag = TRUE;
				continue;
			}
		}

		//if opcodelist
		else if (!strcmp("opcodelist", modified_input[0]) && size == 1) {
			print_opcodeList();
			insertNode(head, save_input);
		}

		//if it is not an available input
		else {
			printf("ERROR: Not an available input. see h[elp].\n");
			flag = TRUE;
			continue;
		}
	}

	return 0;
}

int isHex(char* input) {
	unsigned int i;
	for (i = 0; i < strlen(input); i++) {
		//if each character is not a hex num
		if (isxdigit(input[i]) == FALSE) return FALSE;
	}
	return TRUE;
}
