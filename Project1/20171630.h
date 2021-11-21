#define FALSE 0
#define TRUE 1
#define MEM_SIZE 1<<20	//2^20, 1MByte(16 X 65536)
#define HASH_TABLE_SIZE 20

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>

//to check if input is hex.
int isHex(char*);

//=========================================
//===========FOR SHELL COMMANDS============
//=========================================

//Node for saving input
typedef struct Node {
	char* data;
	struct Node* next;
}Node;
//head of Linked List for hi[story]
Node* head;

//to add new node at the end of Linked List
void insertNode(Node*, char*);
//to print all nodes in the history list
void ShowAll(Node*);
//to free all memory allocated for history Linked List when q[uit]
void FreeAll(Node*);
//to print the files in the current directory
void printdir();



//=========================================
//===========FOR MEMORY COMMANDS===========
//=========================================

//memory with size 1MByte
int mem[MEM_SIZE];
int end_index;

//for printing out memory values of the given address range
void print_mem(int, int);
//for du[mp]
void dump();
//for du[mp] start
void dumpstart(int);
//for du[mp] start, end
void dumpstartend(int, int);
//for e[dit] address, value
void editvalue(int, int);
//for f[ill] start, end, value
void fillvalue(int, int, int);
//for reset
void reset_mem();



//=========================================
//===========FOR OPCODE COMMANDS===========
//=========================================

//structure for Opcode hashTable
typedef struct Opcode_table {
	int opcode;
	char instruction[10];
	char type[10];
	struct Opcode_table* next;
}Opcode_table;

//hashTable for Opcode table.
Opcode_table** hashTable;
Opcode_table* op_mem;

//for freeing memory allocated for hashTable when q[uit]
void destructor();

//for searching for opcode of the given instruction
int search_opcode(int, char*);

//for opcodelist. prints out all contents of the hashTable
void print_opcodeList();
