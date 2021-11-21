#include "20171630.h"

//free hashTable and op_mem
void destructor() {
	for (int i = 0; i < HASH_TABLE_SIZE; i++) {
		if (hashTable[i] != NULL) free(hashTable[i]);
	}
	free(hashTable);
	free(op_mem);
}

//search for the hashTable opcode instruction. 
int search_opcode(int hash, char* instruction) {
	Opcode_table* cur = hashTable[hash]->next;
	while (cur != NULL) {
		if (!strcmp(instruction, cur->instruction))
			return cur->opcode;
		else cur = cur->next;
	}

	return -1;
}

//print opcode list
void print_opcodeList() {
	int i;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		Opcode_table* cur = hashTable[i]->next;
		printf("%d : ", i);
		if (cur == NULL) {
			printf("\n");
		}
		else {
			while (cur != NULL) {
				printf("[%s,%X]", cur->instruction, cur->opcode);
				if (cur->next != NULL) printf(" -> ");
				else printf("\n");
				cur = cur->next;
			}
		}
	}
}
