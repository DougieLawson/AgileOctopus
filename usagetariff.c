// (C) Copyright 2020-2021, Dougie Lawson. All rights reserved.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <mysql.h>
#include <libconfig.h>
#define CONFIG_FILE "/usr/local/etc/zappi.cfg"

void exit_on_error(char* call, MYSQL *con)
{
	fprintf(stderr, "SQL error %s\n", call);
	if (con != NULL)
	{       
		fprintf(stderr, "%s\n", mysql_error(con));
		mysql_close(con);
	}
	exit(-20);
}

int main(int argc, char* argv[])
{
	char *dateVal;
	int day, mon, year;
	char* fromDate;
	char* toDate;
	dateVal = getenv("QUERY_STRING");
	sscanf(dateVal, "date=%d/%d/%d", &day, &mon, &year);

	char* sql;
	char* sqlTZ;

	const char* d_host;
	const char* d_db;
	const char* d_user;
	const char* d_pwd;

	int param_count, column_count;
	int rc;
	MYSQL_BIND parm[2];
	MYSQL_BIND bind[4];
	MYSQL_STMT *stmtTZ, *stmt;
	MYSQL_RES *prepare_meta_result;
	MYSQL_TIME from_ts;
	MYSQL_TIME to_ts;
	MYSQL_TIME ts;
	unsigned long length[4];
	my_bool is_null[4];
	my_bool error[4];
	my_bool trunc = 0;
	double power_usage;
	double cost;
	double total_usage = 0.0;
	double total_cost = 0.0;
	double price = 0.0;
	char* comma = " ";
	sql="SELECT time_from, power_usage, cost, price FROM Agile_price WHERE time_from >= ? AND time_to <= ? ORDER BY time_from";
//	sqlTZ = "SET time_zone = 'Europe/London';";

        config_t cfg;
        config_setting_t *root;
        config_setting_t *database_g, *db_host, *db_db, *db_pwd, *db_user;

        config_init(&cfg);
        if(! config_read_file(&cfg, CONFIG_FILE))
        {
                fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
                config_destroy(&cfg);
                return(EXIT_FAILURE);
        }

        root = config_root_setting(&cfg);

        database_g = config_setting_get_member(root, "Database");

        db_host = config_setting_get_member(database_g, "sqlhost");
        d_host = config_setting_get_string(db_host);
        db_db = config_setting_get_member(database_g, "sqldbase");
        d_db = config_setting_get_string(db_db);
        db_user = config_setting_get_member(database_g, "sqluser");
        d_user = config_setting_get_string(db_user);
        db_pwd = config_setting_get_member(database_g, "sqlpwd");
        d_pwd = config_setting_get_string(db_pwd);

	from_ts.year = year;
	from_ts.month = mon;
	from_ts.day = day;
	from_ts.hour = 0;
	from_ts.minute = 0;
	from_ts.second = 0;
	from_ts.second_part = 0;

	to_ts.year = year;
	to_ts.month = mon;
	to_ts.day = day;
	to_ts.hour = 23;
	to_ts.minute = 59;
	to_ts.second = 59;
	to_ts.second_part = 999999;

	MYSQL *con = mysql_init(NULL);
	mysql_options(con, MYSQL_REPORT_DATA_TRUNCATION, &trunc);

	// CHANGE ME to use a config file
	if (mysql_real_connect(con, d_host, d_user, d_pwd, d_db, 0, NULL, 0) == NULL)
	{
		exit_on_error("Connect", con);
	}

	stmtTZ = mysql_stmt_init(con);

/*
	if (!stmtTZ) 
	{
		exit_on_error("Stmt_init", con);
	}

	if (mysql_stmt_prepare(stmtTZ, sqlTZ, strlen(sqlTZ)))
	{
		exit_on_error("Stmt prepare", con);
	}

	param_count = mysql_stmt_param_count(stmtTZ);
	if (param_count != 0)
	{
		exit_on_error("Invalid param_count", con);
	}

	prepare_meta_result = mysql_stmt_result_metadata(stmtTZ);

	rc = mysql_stmt_execute(stmtTZ);
	if (rc)
	{
		exit_on_error("Stmt execute", con);
	}

	if (mysql_stmt_close(stmtTZ))
	{
		exit_on_error("Stmt close", con);
	}
*/

	printf("Content-Type: application/json\r\n\r\n{\"ut\":[\n");
//	setenv("TZ", "Europe/London", 1);

	stmt = mysql_stmt_init(con);

	if (!stmt) 
	{
		exit_on_error("Stmt_init", con);
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		exit_on_error("Stmt prepare", con);
	}

	param_count = mysql_stmt_param_count(stmt);
	if (param_count != 2)
	{
		exit_on_error("Invalid param_count", con);
	}
	
	memset(parm, 0, sizeof(parm));

	parm[0].buffer_type = MYSQL_TYPE_DATETIME;
	parm[0].buffer = &from_ts;
//	is_null[0] = (my_bool)0;
//	bind[0].is_null = is_null[0];
//	bind[0].error = &error[0];

	parm[1].buffer_type = MYSQL_TYPE_DATETIME;
	parm[1].buffer = &to_ts;
//	is_null[1] = (my_bool)0;
//	bind[1].is_null = is_null[1];
//	bind[1].error = &error[1];

	if (mysql_stmt_bind_param(stmt, parm))
	{
		exit_on_error("Stmt bind param", con);
	}

	prepare_meta_result = mysql_stmt_result_metadata(stmt);

	rc = mysql_stmt_execute(stmt);
	if (rc)
	{
		exit_on_error("Stmt execute", con);
	}

	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_DATETIME;
	bind[0].buffer = (char *)&ts;
	bind[0].is_null = &is_null[0];
	bind[0].length = &length[0];
	bind[0].error = &error[0];

	bind[1].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[1].buffer = (float *)&power_usage;
	bind[1].is_null = &is_null[1];
	bind[1].length = &length[1];
	bind[1].error = &error[1];

	bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[2].buffer = (float *)&cost;
	bind[2].is_null = &is_null[2];
	bind[2].length = &length[2];
	bind[2].error = &error[2];

	bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[3].buffer = (float *)&price;
	bind[3].is_null = &is_null[3];
	bind[3].length = &length[3];
	bind[3].error = &error[3];


	if (mysql_stmt_bind_result(stmt, bind))
	{
		exit_on_error("Stmt bind result", con);
	}

	rc = mysql_stmt_fetch(stmt);
	while(!rc)
	{
		printf("%s{", comma);
		printf("\"periodStart\":");
		printf("\"%02d-%02d %02d:%02d\"", ts.month, ts.day, ts.hour, ts.minute);

		printf(",\"power_usage\":%f", power_usage);
		printf(",\"cost\":%f", cost);
		printf(",\"price\":%f", price);
		printf("}\n");
	 
		comma=",";
		total_usage += power_usage;
		total_cost += cost;
		rc = mysql_stmt_fetch(stmt);
	}

	mysql_free_result(prepare_meta_result);
	if (mysql_stmt_close(stmt))
	{
		exit_on_error("Stmt close", con);
	}

	mysql_close(con);
	mysql_library_end();

	printf("],\n");
	printf("\"total_usage\":%f,", total_usage);
	printf("\"total_cost\":%f }", total_cost);
	return 0;
}
