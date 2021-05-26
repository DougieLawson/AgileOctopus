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

  	int param_count, column_count;
  	int rc, execrc, fetchrc, alldone;
	int resultCount;
	MYSQL_BIND params[2];
  	MYSQL_BIND returned[4];
  	MYSQL_STMT *stmtTZ, *stmt;
  	MYSQL_RES *prepare_meta_result;
  	MYSQL_TIME qry_s;
  	MYSQL_TIME qry_e;
  	MYSQL_TIME ts_s;
  	MYSQL_TIME ts_e;
	MYSQL_TIME ts_l;
	MYSQL_TIME ts_tmp;
	MYSQL_TIME ts_min;
  	unsigned long length[4];
  	my_bool is_null[4];
  	my_bool error[4];
  	my_bool trunc = 0;
  	double price;
	double priceSum = 0.0;
	double priceMin = 600.0;
	time_t now;
	time_t later;
	struct tm *start;
	struct tm *end;

        config_t cfg;
        config_setting_t *root;
        config_setting_t *database_g, *db_host, *db_db, *db_pwd, *db_user;

	now = time(NULL);
	later = now + (9001); // 2hrs 31mins 1second

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

	sql="SELECT period_start, period_end, price, CONVERT_TZ(period_start, 'GMT', 'Europe/London') as local_start FROM Agile_tariffs WHERE period_end > ? AND  period_start <= ? ORDER BY period_start;"; 

	MYSQL *con = mysql_init(NULL);
	mysql_options(con, MYSQL_REPORT_DATA_TRUNCATION, &trunc);

	if (mysql_real_connect(con, d_host, d_user, d_pwd, d_db, 0, NULL, 0) == NULL)
	{
		exit_on_error("Connect", con);
	}
  
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

	memset(params, 0, sizeof(params));
	memset(returned, 0, sizeof(returned));

	start = localtime(&now);
	//printf("Start time is: %02d:%02d:%02d\n", start->tm_hour, start->tm_min, start->tm_sec);
	
	qry_s.year = start->tm_year+1900; 
	qry_s.month = start->tm_mon+1;
	qry_s.day = start->tm_mday;
	qry_s.hour = start->tm_hour;
	qry_s.minute = start->tm_min; 
	qry_s.second = start->tm_sec;
	qry_s.second_part = 0;

	end = localtime(&later);
	//printf("End time is: %02d:%02d:%02d\n", end->tm_hour, end->tm_min, end->tm_sec);

	qry_e.year = end->tm_year+1900;
	qry_e.month = end->tm_mon+1;
	qry_e.day = end->tm_mday;
	qry_e.hour = end->tm_hour;
	qry_e.minute = end->tm_min;
	qry_e.second = end->tm_sec;
	qry_e.second_part = 0;

	params[0].buffer_type = MYSQL_TYPE_DATETIME;
	params[0].buffer = (char *)&qry_s;
	params[0].is_null = 0;
	params[0].length = 0;
	
	params[1].buffer_type = MYSQL_TYPE_DATETIME;
	params[1].buffer = (char *)&qry_e;
	params[1].is_null = 0;
	params[1].length = 0;

	do {
		if (mysql_stmt_bind_param(stmt, params))
		{
			exit_on_error("Stmt bind param", con);
		}

		prepare_meta_result = mysql_stmt_result_metadata(stmt);

		//printf("QRY: from %02d/%02d %02d:%02d:%02d ", qry_s.day, qry_s.month, qry_s.hour, qry_s.minute, qry_s.second);
		//printf("until %02d/%02d %02d:%02d:%02d\n", qry_e.day, qry_e.month, qry_e.hour, qry_e.minute, qry_e.second);
		resultCount = 0;
		execrc = mysql_stmt_execute(stmt);
		if (execrc)
		{
			exit_on_error("Stmt execute", con);
		}
	
		returned[0].buffer_type = MYSQL_TYPE_DATETIME;
		returned[0].buffer = (char *)&ts_s;
		returned[0].is_null = &is_null[0];
		returned[0].length = &length[0];
		returned[0].error = &error[0];
	
		returned[1].buffer_type = MYSQL_TYPE_DATETIME;
		returned[1].buffer = (char *)&ts_e;
		returned[1].is_null = &is_null[1];
		returned[1].length = &length[1];
		returned[1].error = &error[1];

		returned[2].buffer_type = MYSQL_TYPE_DOUBLE;
		returned[2].buffer = (float *)&price;
		returned[2].is_null = &is_null[2];
		returned[2].length = &length[2];
		returned[2].error = &error[2]; 

		returned[3].buffer_type = MYSQL_TYPE_DATETIME;
		returned[3].buffer = (char *)&ts_l;
		returned[3].is_null = &is_null[3];
		returned[3].length = &length[3];
		returned[3].error = &error[3];

		if (mysql_stmt_bind_result(stmt, returned))
		{
			exit_on_error("Stmt returned result", con);
		}

		fetchrc = mysql_stmt_fetch(stmt);
		ts_tmp = ts_l;
		alldone = fetchrc;

		while(!fetchrc)
		{
			resultCount++;
			//printf("TS: from %02d/%02d %02d:%02d:%02d ", ts_s.day, ts_s.month, ts_s.hour, ts_s.minute, ts_s.second);
			//printf("until %02d/%02d %02d:%02d:%02d ", ts_e.day, ts_e.month, ts_e.hour, ts_e.minute, ts_e.second);
			//printf("price %f\n", price);
			priceSum += price;
			fetchrc = mysql_stmt_fetch(stmt);
			if (resultCount == 1) 
			{
				qry_s = ts_s;
			}
			qry_e = ts_e;
		}
		//printf("--- SUM %f --------------- RC: %d ", priceSum, resultCount);
		if (resultCount < 6) alldone = fetchrc;
		else {
			if (priceSum < priceMin)
			{
				priceMin = priceSum;
				ts_min = ts_tmp;
				//printf(" MIN %f", priceMin);
			}
			priceSum = 0.0;
		}
		fetchrc = 0;
		//printf("\n");

	} while(!alldone);


	printf("MIN: %02d/%02d %02d:%02d:%02d ", ts_min.day, ts_min.month, ts_min.hour, ts_min.minute, ts_min.second);
	printf("Avg price %f\n", priceMin / 6.0);

	mysql_free_result(prepare_meta_result);
	if (mysql_stmt_close(stmt))
	{
		exit_on_error("Stmt close", con);
	}

	mysql_close(con);
	mysql_library_end();

	return 0;
}
