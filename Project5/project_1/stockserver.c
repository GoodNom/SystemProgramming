/*
 * echoserveri.c - An iterative echo server
 */
 /* $begin echoserverimain */
#include "csapp.h"

typedef struct { /* Represents a pool of connected descriptors */
	int maxfd; /* Largest descriptor in read_set */
	fd_set read_set; /* Set of all active descriptors */
	fd_set ready_set; /* Subset of descriptors ready for reading */
	int nready; /* Number of ready descriptors from select */
	int maxi; /* High water index into client array */
	int clientfd[FD_SETSIZE]; /* Set of active descriptors */
	rio_t clientrio[FD_SETSIZE]; /* Set of active read buffers */
} pool;

struct item setItem(int ID, int left_stock, int price);
Node* setNode(struct item data, Node* leftChild, Node* rightChild);
Node* addNode(Node* node, struct item data);
Node* searchTree(Node* ptr, int key);
void showTree(Node* node);
void deleteTree(Node* node);
void storeTxt(FILE* fp, Node* node);


void show(int connfd, Node* node);
void buy(int connfd, Node* node, int ID, int n);
void sell(int connfd, Node* node, int ID, int n);

void init_pool(int listenfd, pool* p);
void add_client(int connfd, pool* p);
void check_clients(pool* p);

Node* root = NULL;

int main(int argc, char** argv)
{
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
	char client_hostname[MAXLINE], client_port[MAXLINE];
	struct item temp;
	static pool pool;

	int id, left, price;
	FILE* fp = fopen("stock.txt", "r");

	if (fp == NULL) {
		fprintf(stderr, "The file does not exist.\n");
		exit(0);
	}

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	while (!feof(fp)) {
		fscanf(fp, "%d %d %d", &id, &left, &price);
		temp = setItem(id, left, price);
		root = addNode(root, temp);
	}
	fclose(fp);

	listenfd = Open_listenfd(argv[1]);
	init_pool(listenfd, &pool);


	while (1) {
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);
		if (FD_ISSET(listenfd, &pool.ready_set)) {
			clientlen = sizeof(struct sockaddr_storage);
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
			Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE,
				client_port, MAXLINE, 0);
			printf("Connected to (%s, %s)\n", client_hostname, client_port);
			add_client(connfd, &pool);
			
		}
		check_clients(&pool);
	}

	deleteTree(root);
	exit(0);
}
/* $end echoserverimain */


/* Item set */
struct item setItem(int ID, int left_stock, int price) {
	struct item data;
	data.ID = ID;
	data.left_stock = left_stock;
	data.price = price;
	return data;
}

/* Node set */
Node* setNode(struct item data, Node* leftChild, Node* rightChild) {
	Node* node = (Node*)malloc(sizeof(Node));
	node->data = data;
	node->leftChild = leftChild;
	node->rightChild = rightChild;
	return node;
}

/* Node add */
Node* addNode(Node* node, struct item data) {
	if (node == NULL) {
		node = setNode(data, NULL, NULL);
	}
	else if (data.ID < node->data.ID) {
		node->leftChild = addNode(node->leftChild, data);
	}
	else if (data.ID > node->data.ID) {
		node->rightChild = addNode(node->rightChild, data);
	}
	return node;
}

/* Node search */
Node* searchTree(Node* ptr, int key) {
	while (ptr != NULL && ptr->data.ID != key)
	{
		if (ptr->data.ID > key)
		{
			ptr = ptr->leftChild;
		}
		else
		{
			ptr = ptr->rightChild;
		}
	}
	return ptr;
}

/* all Node print */
void showTree(Node* node) {
	if (node != NULL) {
		showTree(node->leftChild);
		printf("%d %d %d\n", node->data.ID, node->data.left_stock, node->data.price);
		showTree(node->rightChild);
	}
}

/* all free Node */
void deleteTree(Node* node) {
	if (node != NULL) {
		deleteTree(node->leftChild);
		deleteTree(node->rightChild);
		free(node);
	}
}

/* .txt tree store */
void storeTxt(FILE* fp, Node* node) {
	if (node != NULL) {
		storeTxt(fp, node->leftChild);
		fprintf(fp, "%d %d %d\n", node->data.ID, node->data.left_stock, node->data.price);
		storeTxt(fp, node->rightChild);
	}
}

void show(int connfd, Node* node) {
	char buf[1024];
	if (node != NULL) {
		show(connfd, node->leftChild);

		sprintf(buf, "%d %d %d\n", node->data.ID, node->data.left_stock, node->data.price);
		Rio_writen(connfd, buf, strlen(buf));

		show(connfd, node->rightChild);
	}
}

void buy(int connfd, Node* node, int ID, int n) {
	Node* stock = searchTree(node, ID);
	char buf[1024];
	if ((stock->data.left_stock - n) >= 0) {
		stock->data.left_stock -= n;
		sprintf(buf, "[buy] \033[1;36msuccess\033[0m\n");
		Rio_writen(connfd, buf, strlen(buf));
	}
	else {
		sprintf(buf, "Not enough left stock\n");
		Rio_writen(connfd, buf, strlen(buf));
	}
}

void sell(int connfd, Node* node, int ID, int n) {
	Node* stock = searchTree(node, ID);
	char buf[1024];
	stock->data.left_stock += n;
	sprintf(buf, "[sell] \033[1;36msuccess\033[0m\n");
	Rio_writen(connfd, buf, strlen(buf));
}

void init_pool(int listenfd, pool* p)
{
	/* Initially, there are no connected descriptors */
	int i;
	p->maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
		p->clientfd[i] = -1;

	/* Initially, listenfd is only member of select read set */
	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool* p)
{
	int i;
	p->nready--;
	for (i = 0; i < FD_SETSIZE; i++) /* Find an available slot */
		if (p->clientfd[i] < 0) {
			/* Add connected descriptor to the pool */
			p->clientfd[i] = connfd;
			Rio_readinitb(&p->clientrio[i], connfd);

			/* Add the descriptor to descr iptor set */
			FD_SET(connfd, &p->read_set);
			/* Update max descriptor and pool high water mark */
			if (connfd > p->maxfd)
				p->maxfd = connfd;
			if (i > p->maxi)
				p->maxi = i;
			break;
		}
	if (i == FD_SETSIZE) /* Couldn't find an empty slot */
		app_error("add_client error : Too many clients");
}

void check_clients(pool* p)
{
	int i, connfd, n;
	char buf[MAXLINE];
	rio_t rio;
	char cmd[10];
	int id, num;
	for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
		connfd = p->clientfd[i];
		rio = p->clientrio[i];
		/* If the descriptor is ready, echo a text line from i t */
		if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
			p->nready--;
			if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
				cmd[0] = '\0'; id = 0; num = 0;
				sscanf(buf, "%s %d %d", cmd, &id, &num);
				printf("server received %d bytes(%d)\n", n,connfd);
				if (!strcmp(cmd, "show")) {
					show(connfd, root);
					Rio_writen(connfd, "end\n", 4);
				}
				else if (!strcmp(cmd, "buy")) {
					buy(connfd, root, id, num);
				}
				else if (!strcmp(cmd, "sell")) {
					sell(connfd, root, id, num);
				}
				else {
					Rio_writen(connfd, buf, n);
				}
			}

			/* EOF detected, remove descriptor from pool */
			else {
				Close(connfd);
				FD_CLR(connfd, &p->read_set);
				p->clientfd[i] = -1;
				printf("disconnected %d\n", connfd);
				FILE *fp2 = fopen("stock.txt", "w");
				if (fp2 == NULL) {
					perror("Failed to open the source");
				}
				else {
					storeTxt(fp2, root);
					fclose(fp2);
				}
			}
		}
	}
}