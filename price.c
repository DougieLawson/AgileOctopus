/* 
 Copyright © Dougie Lawson, 2020-2021, All rights reserved 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <libconfig.h>
#include <mysql.h>
#include <libgen.h>
#define CONFIG_FILE "/usr/local/etc/zappi.cfg"
#define COLUMNS 2

char *comma = " ";

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
	int rc;
	int param_count, column_count;
	MYSQL_STMT *stmtTZ, *stmt;
	MYSQL_BIND bind[COLUMNS];
	MYSQL_RES *prepare_meta_result;
	MYSQL_TIME ts;
	MYSQL_TIME ps;
	double price;
	float imported, zappiimp, zappidiv, house, charging;
       	float t_imported, t_charging, t_house, t_cost;	
	unsigned long length[COLUMNS];
	my_bool is_null[COLUMNS];
	my_bool error[COLUMNS];
	my_bool trunc = 0;
	char *sqlTZ, *sql;
	const char* d_host;
	const char* d_db;
	const char* d_user;
	const char* d_pwd;

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

	t_imported = 0;
	t_charging = 0;
	t_house = 0;
	t_cost = 0;

	setenv("TZ", "Europe/London", 1);

	MYSQL *con = mysql_init(NULL);
	mysql_options(con, MYSQL_REPORT_DATA_TRUNCATION, &trunc);

	if( con == NULL )
	{
		fprintf(stderr, "Can't run Mariadb");
		exit_on_error("Really bad", NULL);
	}

	if (mysql_real_connect(con, d_host, d_user, d_pwd, d_db, 0, NULL, 0) == NULL)
	{
		exit_on_error("Connect", con);
	}

	sqlTZ = "SET time_zone = 'Europe/London';";
	stmtTZ = mysql_stmt_init(con);

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

	stmt = mysql_stmt_init(con);
	if (!stmt) 
	{
		exit_on_error("Stmt init", con);
	}

	sql= " SELECT period_start, price from Agile_tariffs ORDER BY period_start ";

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		exit_on_error("Stmt prepare", con);
	}

	param_count = mysql_stmt_param_count(stmt);
	if (param_count != 0)
	{
		exit_on_error("Invalid param count", con);
	}

	prepare_meta_result = mysql_stmt_result_metadata(stmt);
	if (!prepare_meta_result)
	{
		exit_on_error("Stmt result metadata", con);
	}

	rc = mysql_stmt_execute(stmt);
	if (rc)
	{
		exit_on_error("Stmt execute", con);
	}

	column_count = mysql_num_fields(prepare_meta_result);
	if (column_count != COLUMNS)
	{
		exit_on_error("Invalid column count", con);
	}

	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_DATETIME;
	bind[0].buffer = (char *)&ps;
	bind[0].is_null = &is_null[0];
	bind[0].length = &length[0];
	bind[0].error = &error[0];

	bind[1].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[1].buffer = (double *)&price;
	bind[1].is_null = &is_null[1];
	bind[1].length = &length[1];
	bind[1].error = &error[1];

	if (mysql_stmt_bind_result(stmt, bind))
	{
		exit_on_error("Stmt bind result", con);
	}

	rc = mysql_stmt_fetch(stmt);
	while(!rc)
	{
		printf("Period: %02d:%02d ", ps.hour, ps.minute);
		printf("Price (kWm): %2.2fp \n", price);
		rc = mysql_stmt_fetch(stmt);
	}

	mysql_free_result(prepare_meta_result);
	if (mysql_stmt_close(stmt))
	{
		exit_on_error("Stmt close", con);
	}

	mysql_close(con);
	mysql_library_end();

	return 0;
}
