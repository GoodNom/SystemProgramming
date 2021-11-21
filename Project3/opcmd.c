#include "20171630.h"

//free hashTable and op_mem
void destructor() {
	int i;
	Opcode_table* node;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		while (hashTable[i] != NULL) {
			node = hashTable[i];
			hashTable[i] = hashTable[i]->next;
			free(node);		
		} 
	}
}

//search for the hashTable opcode instruction. 
int search_opcode(char* instruction) {
	int hash = hashFunction(instruction);
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
