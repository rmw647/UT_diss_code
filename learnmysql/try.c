#include <my_global.h>
#include <mysql.h>

#define SFREQ 	7

static void print_error(MYSQL *conn, char *message);

int main (int argc,char *argv[])	{
	MYSQL *conn;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int count,i;

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
	
	mysql_query(conn,"SELECT COUNT(*) FROM strategies");
	result = mysql_store_result(conn);

	row = mysql_fetch_row(result);

	printf("%s\n",row[0]);

	count = atoi(row[0]);

	mysql_query(conn,"ALTER TABLE strategies ADD COLUMN strat_id INT");
	for (i=0;i<count;i++)
		mysql_query(conn,"INSERT INTO strategies(strat_id) VALUES (i)");

	mysql_free_result(result);	

	mysql_close(conn);
}

static void print_error(MYSQL *conn, char *message)     {
        fprintf(stderr,"%s\n",message);
        if (conn!=NULL) {
                fprintf(stderr,"Error %u (%s): %s\n",
                        mysql_errno(conn),mysql_sqlstate(conn),mysql_error(conn));
        }
}
