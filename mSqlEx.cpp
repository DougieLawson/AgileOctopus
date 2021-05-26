/* 
 Copyright © Dougie Lawson, 2020-2021, All rights reserved 
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include "mysql/mysql.h"
#define DBSERVER "192.168.3.14"
#define DBNAME "mysqluser"
#define DBPASS "mysqlpwd"
#define DBN "Octo"

using namespace std;

int main() 
{
	MYSQL *Connection;
	MYSQL_BIND param[1];
	MYSQL_BIND bind[3];
	MYSQL_RES *res_set;
	MYSQL_RES *prepare_meta_result;
	MYSQL_ROW row;
  	MYSQL_STMT *stmt;
  	my_bool trunc = 0;
  	unsigned long length[3];
  	my_bool is_null[3];
  	my_bool error[3];
	double maxPrice;
	double price;
	MYSQL_TIME ts_s;
	MYSQL_TIME ts_e;
	int rc;

	const char* sql = "SELECT CONVERT_TZ(period_start, 'UTC', 'Europe/London') as period_start, CONVERT_TZ(period_end, 'UTC', 'Europe/London') as period_end,  price FROM Agile_tariffs WHERE period_start >= NOW() AND price BETWEEN (select MIN(price) FROM Agile_tariffs WHERE period_start >= NOW()) and ?;";

	Connection = mysql_init(NULL);
	Connection = mysql_real_connect(Connection, DBSERVER, DBNAME, DBPASS, DBN, 0, NULL, 0);
	if (!Connection) 
	{
		cout << "Conn failed" << endl;
		return(EXIT_FAILURE);	
	}

	mysql_options(Connection, MYSQL_REPORT_DATA_TRUNCATION, &trunc);
	stmt = mysql_stmt_init(Connection);
	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		cout << "Stmt prepare" << endl;
		return(EXIT_FAILURE);	
	}

	maxPrice = 10.5;
	memset(param, 0, sizeof(param));
	param[0].buffer_type = MYSQL_TYPE_DOUBLE; 
	param[0].buffer = (double *)&maxPrice;
	param[0].is_null = 0;
	param[0].length = 0;
	param[0].error = 0;

	if (mysql_stmt_bind_param(stmt, param))
	{
		cout << "Stmt bind param" << endl;
		return(EXIT_FAILURE);	
	}

	rc = mysql_stmt_execute(stmt);
	prepare_meta_result = mysql_stmt_result_metadata(stmt);
	rc = mysql_stmt_store_result(stmt);

	memset(bind, 0, sizeof(bind));
	
	bind[0].buffer_type = MYSQL_TYPE_DATETIME;
	bind[0].buffer = (char *)&ts_s;
	bind[0].is_null = &is_null[0];
	bind[0].length = &length[0];
	bind[0].error = &error[0];
	
	bind[1].buffer_type = MYSQL_TYPE_DATETIME;
	bind[1].buffer = (char *)&ts_e;
	bind[1].is_null = &is_null[1];
	bind[1].length = &length[1];
	bind[1].error = &error[1];

	bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[2].buffer = (float *)&price;
	bind[2].is_null = &is_null[2];
	bind[2].length = &length[2];
	bind[2].error = &error[2];

	if (mysql_stmt_bind_result(stmt, bind))
	{
		cout <<	"Stmt bind result" << endl;
	}
	rc = mysql_stmt_fetch(stmt);
	while(!rc)
	{
		cout << price << " "; 
		cout << setfill('0') << setw(2) << ts_s.hour << ":"; 
		cout << setfill('0') << setw(2) << ts_s.minute << " ";
		cout << setfill('0') << setw(2) << ts_e.hour << ":";
		cout << setfill('0') << setw(2) << ts_e.minute << endl;
		rc = mysql_stmt_fetch(stmt);
	}

	mysql_close(Connection);	
	return 0;
}
