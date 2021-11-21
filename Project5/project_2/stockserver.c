/*
 * echoserveri.c - An iterative echo server
 */
 /* $begin echoserverimain */
#include "csapp.h"
#define NTHREADS  16
#define SBUFSIZE  32

struct item setItem(int ID, int left_stock, int price, int readcnt);
Node* setNode(struct item data, Node* leftChild, Node* rightChild);
Node* addNode(Node* node, struct item data);
Node* searchTree(Node* ptr, int key);
void showTree(Node* node);
void deleteTree(Node* node);
void storeTxt(FILE* fp, Node* node);


void show(int connfd, Node* node);
void buy(int connfd, Node* node, int ID, int n);
void sell(int connfd, Node* node, int ID, int n);


void echo(int connfd, Node* node);

void* thread(void* vargp);
void sbuf_init(sbuf_t* sp, int n);
void sbuf_deinit(sbuf_t* sp);
void sbuf_insert(sbuf_t* sp, int item);
int sbuf_remove(sbuf_t* sp);


Node* root = NULL;
//FILE* fp2;
sbuf_t sbuf; /* Shared buffer of connected descriptors */
static sem_t mutex;


int main(int argc, char** argv)
{
	int listenfd, connfd, i;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
	char client_hostname[MAXLINE], client_port[MAXLINE];
	pthread_t tid;
	struct item temp;

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
		temp = setItem(id, left, price, 0);
		root = addNode(root, temp);
	}
	fclose(fp);

	//showTree(root);
	Sem_init(&mutex, 0, 1);
	listenfd = Open_listenfd(argv[1]);

	sbuf_init(&sbuf, SBUFSIZE);
	for (i = 0; i < NTHREADS; i++)
		Pthread_create(&tid, NULL, thread, NULL);

	while (1) {
		clientlen = sizeof(struct sockaddr_storage);
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE,
			client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */
	}


	deleteTree(root);
	exit(0);
}
/* $end echoserverimain */



/* Item set */
struct item setItem(int ID, int left_stock, int price, int readcnt) {
	struct item data;
	data.ID = ID;
	data.left_stock = left_stock;
	data.price = price;
	data.readcnt = 0;
	Sem_init(&data.mutex, 0, 1);
	Sem_init(&data.w, 0, 1);
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

/* all Node free */
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
		P(&node->data.mutex);
		node->data.readcnt++;
		if (node->data.readcnt == 1)
			P(&node->data.w);
		V(&node->data.mutex);

		fprintf(fp, "%d %d %d\n", node->data.ID, node->data.left_stock, node->data.price);

		P(&node->data.mutex);
		node->data.readcnt--;
		if (node->data.readcnt == 0)
			V(&node->data.w);
		V(&node->data.mutex);
		storeTxt(fp, node->rightChild);
	}

}

void show(int connfd, Node* node) {

	char buf[1024];
	if (node != NULL) {
		show(connfd, node->leftChild);

		P(&node->data.mutex);
		node->data.readcnt++;
		if (node->data.readcnt == 1)
			P(&node->data.w);
		V(&node->data.mutex);

		sprintf(buf, "%d %d %d\n", node->data.ID, node->data.left_stock, node->data.price);
		Rio_writen(connfd, buf, strlen(buf));

		P(&node->data.mutex);
		node->data.readcnt--;
		if (node->data.readcnt == 0)
			V(&node->data.w);
		V(&node->data.mutex);

		show(connfd, node->rightChild);
	}


}

void buy(int connfd, Node* node, int ID, int n) {


	P(&node->data.w);
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
	V(&node->data.w);

}

void sell(int connfd, Node* node, int ID, int n) {

	P(&node->data.w);
	Node* stock = searchTree(node, ID);
	char buf[1024];
	stock->data.left_stock += n;
	sprintf(buf, "[sell] \033[1;36msuccess\033[0m\n");
	Rio_writen(connfd, buf, strlen(buf));
	V(&node->data.w);

}

void* thread(void* vargp)
{
	Pthread_detach(pthread_self());
	while (1) {
		int connfd = sbuf_remove(&sbuf);	/* Remove connfd from buffer */ 
		echo(connfd, root);					/* Service client */
		Close(connfd);
		//P(&mutex);
		printf("disconnected %d\n", connfd);
		FILE *fp2 = fopen("stock.txt", "w");
		if (fp2 == NULL) {
			perror("Failed to open the source");
		}
		else {
			storeTxt(fp2, root);
			fclose(fp2);
			//showTree(root);
		}
		//V(&mutex);
	}
}

/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t* sp, int n)
{
	sp->buf = Calloc(n, sizeof(int));
	sp->n = n;                       /* Buffer holds max of n items */
	sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
	Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
	Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
	Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t* sp)
{
	Free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t* sp, int item)
{
	P(&sp->slots);                          /* Wait for available slot */
	P(&sp->mutex);                          /* Lock the buffer */
	sp->buf[(++sp->rear) % (sp->n)] = item;   /* Insert the item */
	V(&sp->mutex);                          /* Unlock the buffer */
	V(&sp->items);                          /* Announce available item */
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t* sp)
{
	int item;
	P(&sp->items);                          /* Wait for available item */
	P(&sp->mutex);                          /* Lock the buffer */
	item = sp->buf[(++sp->front) % (sp->n)];  /* Remove the item */
	V(&sp->mutex);                          /* Unlock the buffer */
	V(&sp->slots);                          /* Announce available slot */
	return item;
}
/* $end sbuf_remove */
