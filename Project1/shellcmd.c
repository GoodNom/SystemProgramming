#include "20171630.h"


//for hi[story]
void insertNode(Node* root, char* refined_input) {
	//initialize new node
	Node* node = (Node*)malloc(sizeof(Node));
	node->next = NULL;
	node->data = (char*)malloc(sizeof(char) * 100);
	strcpy(node->data, refined_input);

	if (root->next == NULL) root->next = node;

	else {
		Node* cur = root->next;
		while (cur->next != NULL) cur = cur->next;
		cur->next = node;
	}
}

void ShowAll(Node* root) {
	int index = 1;
	Node* cur = root->next;
	if (cur == NULL) return;
	else {
		while (cur != NULL) {
			printf("\t\t%d\t%s\n", index++, cur->data);
			cur = cur->next;
		}
	}
}

void FreeAll(Node* root) {
	if (root->next == NULL) return;
	Node* cur = root->next;
	while (cur != NULL) {
		Node* next = cur->next;
		free(cur);
		cur = next;
	}
	free(root);
}

//for d[ir]
void printdir() {
	DIR *dp = NULL;
	struct dirent *file = NULL;
	struct stat buf;

	if ((dp = opendir(".")) == NULL) {
		printf("ERROR: Cannot read directory.\n");
		return;
	}

	while ((file = readdir(dp)) != NULL) {

		//'.' And'..' is skipped.
		if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")) continue;

		//print file/directory name first
		printf("\t\t%s", file->d_name);
		stat(file->d_name, &buf);
		//if the file is a directory, S_ISDIR()=1
		if (S_ISDIR(buf.st_mode)) printf("/");
		//.out is the file the user has permission to execute.
		else if ((buf.st_mode & S_IXUSR)) printf("*");
		printf("\n");
	}
	closedir(dp);
}
