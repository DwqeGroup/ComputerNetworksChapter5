//#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<stdio.h>
#include<conio.h>
using namespace std;
#define MAX_ROUTER 20
struct node
{
	unsigned dist[MAX_ROUTER];
	unsigned from[MAX_ROUTER];
}rt[MAX_ROUTER];
int nodes = 5;
int init_table[MAX_ROUTER][MAX_ROUTER] = { { 0,7,10000,10000,10 },{ 7,0,1,10000 ,8 },{ 10000,1,0,2,10000 },{ 10000,10000,2,0,2 },{ 10,8,10000,2,0 } };
void init()
{
	int  i, j;
	for (i = 0; i < nodes; i++)
	{
		for (j = 0; j < nodes; j++)
		{
			rt[i].dist[j] = init_table[i][j];
			rt[i].from[j] = j;
		}
	}
}
void update()
{
	int  i, j, k, count = 0;
	do
	{
		count = 0;
		for (i = 0; i < nodes; i++)
			for (j = 0; j < nodes; j++)
				for (k = 0; k < nodes; k++)
					if (rt[i].dist[j] > init_table[i][k] + rt[k].dist[j])
					{
						rt[i].dist[j] = rt[i].dist[k] + rt[k].dist[j];
						rt[i].from[j] = k;
						count++;
					}
	} while (count != 0);
}
void print_table()
{
	int i, j;
	for (i = 0; i < nodes; i++)
	{
		printf("\nRouter_table of router %d\n", i + 1);
		for (j = 0; j < nodes; j++)
		{
			printf("\tnode:%d  Distance:%d  Next hop:%d\n", j + 1, rt[i].dist[j], rt[i].from[j] + 1);
		}
	}
}
int main()
{
	init();
	update();
	print_table();
	printf("Input 'c' to change init_table, input other words to exit:\n");
	char m;
	while ((m=getchar()-'c') == 0)
	{
		printf("Please input three numbers: start_node next_node distance\n");
		printf("Attention: node number should in[0,%d), distance should in [0,100)\n", nodes);
		printf("After input all changes, input any word to end input.\n");
		int lx, ly, val;
		while (scanf("%d %d %d", &lx, &ly, &val) == 3)
		{
			if (lx < nodes&&lx >= 0 && ly >= 0 && ly < nodes)
			{
				init_table[lx][ly] = val;
				init_table[ly][lx] = val;
			}
			else
				printf("The node number should between 0 and %d.\n", nodes);

		}
		getchar();
		getchar();
		init();
		update();
		print_table();
		printf("Input 'c' to change init_table, input other words to exit:\n");
	}
	//system("pause");
}