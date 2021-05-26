// (C) Copyright 2020-2021, Dougie Lawson. All rights reserved.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <mysql.h>
#include <time.h>
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

void bestJSON(MYSQL* con)
{
	printf("\"PriceData\":[");
  	char *sql;

  	int param_count, column_count;
  	int rc, execrc, fetchrc, alldone;
	int resultCount;
	MYSQL_BIND params[2];
  	MYSQL_BIND returned[4];
  	MYSQL_STMT *stmt;
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

	now = time(NULL);
	later = now + (9001); // 2hrs 31mins 1second

	sql="SELECT period_start, period_end, price, CONVERT_TZ(period_start, 'GMT', 'Europe/London') as local_start FROM Agile_tariffs WHERE period_end > ? AND  period_start <= ? ORDER BY period_start;"; 

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
	
	qry_s.year = start->tm_year+1900; 
	qry_s.month = start->tm_mon+1;
	qry_s.day = start->tm_mday;
	qry_s.hour = start->tm_hour;
	qry_s.minute = start->tm_min; 
	qry_s.second = start->tm_sec;
	qry_s.second_part = 0;

	end = localtime(&later);

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
			priceSum += price;
			fetchrc = mysql_stmt_fetch(stmt);
			if (resultCount == 1) 
			{
				qry_s = ts_s;
			}
			qry_e = ts_e;
		}
		if (resultCount < 6) alldone = fetchrc;
		else {
			if (priceSum < priceMin)
			{
				priceMin = priceSum;
				ts_min = ts_tmp;
			}
			priceSum = 0.0;
		}
		fetchrc = 0;

	} while(!alldone);

	printf("{\"MIN\":\"%02d/%02d %02d:%02d:%02d\",\"Avgprice\":%.2f}", ts_min.day, ts_min.month, ts_min.hour, ts_min.minute, ts_min.second, priceMin / 6.0);

	mysql_free_result(prepare_meta_result);
	if (mysql_stmt_close(stmt))
	{
		exit_on_error("Stmt close", con);
	}
	printf("]");

}

int getHHTariff(MYSQL* con)
{

	printf("\"ChartData\":[");
	char *sql;
   	char *sqlTZ;

	int param_count, column_count;
	int rc;
	MYSQL_BIND bind[3];
	MYSQL_STMT *stmtTZ, *stmt;
	MYSQL_RES *prepare_meta_result;
	MYSQL_TIME ts;
	unsigned long unixts;
	unsigned long length[3];
	my_bool is_null[3];
	my_bool error[3];
	double price;
	char *comma = " ";
	char backgroundColour[40];
	char density[20];
       	time_t now = time(NULL);
     	struct tm *localNow = localtime(&now);
	int nowHH;

	long unixNow = mktime(localNow);

	sql="SELECT CONVERT_TZ(period_start, 'UTC', 'Europe/London') as period_start, UNIX_TIMESTAMP(CONVERT_TZ(period_start, 'UTC', 'Europe/London')), price FROM Agile_tariffs WHERE (NOW() - INTERVAL 12 HOUR) <= period_start AND (NOW() + INTERVAL 12 HOUR) >= period_start ORDER BY period_start;";
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
		exit_on_error("Stmt_init", con);
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
	{
		exit_on_error("Stmt prepare", con);
	}

	param_count = mysql_stmt_param_count(stmt);
	if (param_count != 0)
	{
		exit_on_error("Invalid param_count", con);
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

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = &unixts;
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

	rc = mysql_stmt_fetch(stmt);
	while(!rc)
	{
		printf("%s{", comma);
		printf("\"periodStart\":");
		printf("\"%02d-%02d %02d:%02d\"", ts.month, ts.day, ts.hour, ts.minute);

		nowHH = localNow->tm_min> 29 ? 30 : 0;
		sprintf(density,"0.2");
		if (unixts <= unixNow) 
		{
			sprintf(density,"0.1");
		}
		if (unixts >= unixNow) 
		{
			sprintf(density, "0.3");
		}

		if (ts.hour == localNow->tm_hour && ts.minute == nowHH)
		{
			sprintf(density,"1.0");
		}
		printf(",\"price\":%.2f", price);

  		if (price > 25.0) { sprintf(backgroundColour,"255,0,0"); }
  		else if (price > 20.0) {sprintf(backgroundColour,"223,54,2"); }
  	       	else if (price > 15.0) { sprintf(backgroundColour,"211,110,0"); }
  	       	else if (price > 10.0) { sprintf(backgroundColour,"215,183,3"); }
   	      	else if (price > 5.0) { sprintf(backgroundColour,"109,253,30"); }
  		else if (price > 0.0) { sprintf(backgroundColour,"14 ,198 ,14"); }
  		else if (price > -5.0) { sprintf(backgroundColour,"6,196,191"); }
  		else if (price > -10.0) { sprintf(backgroundColour,"0,0,255"); }
  		else { sprintf(backgroundColour,"0 ,0 ,139"); }
		printf(",\"backgroundColor\":\"rgba(%s, %s)\"", backgroundColour, density);
		printf(",\"borderColor\":\"rgb(%s)\"",backgroundColour);
		printf("}\n");
 
		comma=",";
		rc = mysql_stmt_fetch(stmt);
	}

	mysql_free_result(prepare_meta_result);
	if (mysql_stmt_close(stmt))
	{
		exit_on_error("Stmt close", con);
	}
	printf("]");
}


int main(int argc, char* argv[])
{

	const char* d_host;
	const char* d_db;
	const char* d_user;
	const char* d_pwd;

	my_bool trunc = 0;

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

	MYSQL *con = mysql_init(NULL);
	mysql_options(con, MYSQL_REPORT_DATA_TRUNCATION, &trunc);

	// CHANGE ME to use a config file
	if (mysql_real_connect(con, d_host, d_user, d_pwd, d_db, 0, NULL, 0) == NULL)
	{
		exit_on_error("Connect", con);
	}

	printf("Content-Type: application/json\r\n\r\n{\n");
	setenv("TZ", "Europe/London", 1);
	getHHTariff(con);
	printf(",\n");
	bestJSON(con);
	
	mysql_close(con);
	mysql_library_end();

	printf("}\n");
	return 0;
}
