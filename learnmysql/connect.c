#include <my_global.h>
#include <mysql.h>

#define SFREQ 	7

static void print_error(MYSQL *conn, char *message);

int main (int argc,char *argv[])	{
	MYSQL *conn;
	MYSQL_RES *results;
	MYSQL_ROW row;

	conn = mysql_init(NULL);
	if (conn  == NULL)	{
		print_error(conn, "Cannot initialize MySQL");
		exit(1);
	}
	if(mysql_real_connect(conn, "localhost", "root", NULL, "testdb", 0, NULL, 0)==NULL)	{
     		 print_error(conn, "Cannot connect");
     		 exit(1);
  	 }
/*	mysql_query(conn, "CREATE TABLE safeguards(name VARCHAR(25),freq INT,dep INT, fap FLOAT)");*/

/*	mysql_query(conn,"INSERT INTO safeguards VALUES('NDA',7,NULL,0.01)");*/
/*	mysql_query(conn,"INSERT INTO safeguards VALUES('Inventory',7,1,NULL)");*/
	mysql_query(conn,"INSERT INTO safeguards VALUES('Pseals',7,NULL,NULL)");
	mysql_query(conn,"INSERT INTO safeguards VALUES('Mbalance',7,NULL,0.01)");	

	mysql_close(conn);
}

static void print_error(MYSQL *conn, char *message)     {
        fprintf(stderr,"%s\n",message);
        if (conn!=NULL) {
                fprintf(stderr,"Error %u (%s): %s\n",
                        mysql_errno(conn),mysql_sqlstate(conn),mysql_error(conn));
        }
}
