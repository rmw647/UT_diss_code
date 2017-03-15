#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "/usr/local/mysql/include/mysql.h"

#define NUMSG		3

/*allowable values for defender parameters; game picks from these values*/
int status[2] = {0,1};					/*0-active,1-not active-- defender and attacker*/
float fap[2] = {0.01,0.001};			/*1-0.01,2-0.001*/
int freq[2] = {7,30};					/*scheduled inspection frequency*/
int dep[2] = {1,19};					/*dependency*/


int main(int argc, char *argv)	{
	MYSQL *mysql;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	if (mysql_init(mysql) == NULL)	{
		fprintf(stderr, "Cannot initialize MySQL");
		exit(EXIT_FAILURE);
	}


}
