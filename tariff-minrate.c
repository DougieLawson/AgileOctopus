/* 
 Copyright © Dougie Lawson, 2020-2021, All rights reserved 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <mysql.h>
#include <time.h>
#include <libconfig.h>
#include <mosquitto.h>
#define CONFIG_FILE "/home/pi/.zappi.cfg"

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

  	const char* d_host;
  	const char* d_db;
  	const char* d_user;
  	const char* d_pwd;
  	char *sql;
    	char *sqlTZ;

	struct mosquitto *mosq_pub;

  	int param_count, column_count;
  	int rc;
  	MYSQL_BIND bind[3];
	MYSQL_BIND param[1];
  	MYSQL_STMT *stmtTZ, *stmt;
  	MYSQL_RES *prepare_meta_result;
  	MYSQL_TIME ts_s;
  	MYSQL_TIME ts_e;
  	unsigned long length[4];
  	my_bool is_null[4];
  	my_bool error[4];
  	my_bool trunc = 0;
  	double price;
  	double maxPrice;
	char localMQTTmessage[40];

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

	sql="SELECT CONVERT_TZ(period_start, 'UTC', 'Europe/London') as period_start, CONVERT_TZ(period_end, 'UTC', 'Europe/London') as period_end,  price FROM Agile_tariffs WHERE period_start >= NOW() AND price BETWEEN (select MIN(price) FROM Agile_tariffs WHERE period_start >= NOW()) and ?"; 
		
	sqlTZ = "SET time_zone = 'Europe/London';";

	MYSQL *con = mysql_init(NULL);
	mysql_options(con, MYSQL_REPORT_DATA_TRUNCATION, &trunc);

	if (mysql_real_connect(con, d_host, d_user, d_pwd, d_db, 0, NULL, 0) == NULL)
	{
		exit_on_error("Connect", con);
	}

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

	setenv("TZ", "Europe/London", 1);

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
	if (param_count != 1)
	{
		exit_on_error("Invalid param_count", con);
	}

	prepare_meta_result = mysql_stmt_result_metadata(stmt);

	maxPrice = 10.5;
	memset(param, 0, sizeof(param));
	param[0].buffer_type = MYSQL_TYPE_DOUBLE; 
	param[0].buffer = (double *)&maxPrice;
	param[0].is_null = (my_bool)0;
	param[0].length = &length[3];
	param[0].error = &error[3];

	if (mysql_stmt_bind_param(stmt, param))
	{
		exit_on_error("Stmt bind param", con);
	}

		rc = mysql_stmt_execute(stmt);
		if (rc)
		{
			exit_on_error("Stmt execute", con);
		}

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
			exit_on_error("Stmt bind result", con);
		}
//		mosq_pub = mosquitto_new(NULL, true, NULL);
//		mosquitto_connect(mosq_pub, broker, 1883, 0);

		rc = mysql_stmt_fetch(stmt);
		while(!rc)
		{
			printf("%f  ", price);
			printf("%02d:%02d  ", ts_s.hour, ts_s.minute);
			printf("%02d:%02d\n", ts_e.hour, ts_e.minute);
		//	sprintf(localMQTTmessage, "%f" , price);
		//	mosquitto_publish(mosq_pub, NULL, topic, strlen(localMQTTmessage), localMQTTmessage, 0, 0);
			rc = mysql_stmt_fetch(stmt);
		}

//		mosquitto_disconnect(mosq_pub);
//		mosquitto_destroy(mosq_pub);

	mysql_free_result(prepare_meta_result);
	if (mysql_stmt_close(stmt))
	{
		exit_on_error("Stmt close", con);
	}

	mysql_close(con);
	mysql_library_end();

	return 0;
}
