/*
 * echo - read and echo text lines until client closes connection
 */
 /* $begin echo */
#include "csapp.h"

void show(int connfd, Node* node);
void buy(int connfd, Node* node, int ID, int n);
void sell(int connfd, Node* node, int ID, int n);

void echo(int connfd, Node* node)
{
	int n;
	char buf[MAXLINE];
	char cmd[10];
	int id, num;
	rio_t rio;

	Rio_readinitb(&rio, connfd);
	while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		cmd[0] = '\0'; id = 0; num = 0;
		sscanf(buf, "%s %d %d", cmd, &id, &num);
		printf("server received %d bytes(%d)\n", n,connfd);
		if (!strcmp(cmd, "show")) {
			show(connfd, node);
			Rio_writen(connfd, "end\n", 4);
		}
		else if (!strcmp(cmd, "buy")) {
			buy(connfd, node, id, num);
		}
		else if (!strcmp(cmd, "sell")) {
			sell(connfd, node, id, num);
		}
		else {
			Rio_writen(connfd, buf, n);
		}
	}
}
/* $end echo */

