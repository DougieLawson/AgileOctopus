#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <mysql.h>
#include <time.h>

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
	const char* d_host ="192.168.3.14";
	const char* d_db = "EV";
	const char* d_user = "MySQLUser";
	const char* d_pwd = "MySQLPwd";

	char *sql;
   	char *sqlTZ;

	int param_count, column_count;
	int rc;
	MYSQL_BIND bind[2];
	MYSQL_STMT *stmtTZ, *stmt;
	MYSQL_RES *prepare_meta_result;
	MYSQL_TIME ts;
	unsigned long length[2];
	my_bool is_null[2];
	my_bool error[2];
	my_bool trunc = 0;
	double price;
	char *comma = " ";
	char backgroundColour[40];
	char density[20];
       	time_t now = time(NULL);
     	struct tm *localNow = localtime(&now);
	int nowHH;

	sql="SELECT CONVERT_TZ(period_start, 'UTC', 'Europe/London') as period_start, price FROM Agile_tariffs WHERE (NOW() - INTERVAL 12 HOUR) <= period_start AND (NOW() + INTERVAL 12 HOUR) >= period_start ORDER BY period_start;";
	sqlTZ = "SET time_zone = 'Europe/London';";

	MYSQL *con = mysql_init(NULL);
	mysql_options(con, MYSQL_REPORT_DATA_TRUNCATION, &trunc);

	// CHANGE ME to use a config file
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

	printf("Content-Type: application/json\r\n\r\n[\n");
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

	bind[1].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[1].buffer = (float *)&price;
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
		printf("%s{", comma);
		printf("\"periodStart\":");
		printf("\"%02d-%02d %02d:%02d\"", ts.month, ts.day, ts.hour, ts.minute);

	nowHH = localNow->tm_min> 29 ? 30 : 0;
	if (ts.hour == localNow->tm_hour && ts.minute == nowHH)
	{
		sprintf(density,"1.0");
	}
	else 
	{
		sprintf(density,"0.2");
	}

		printf(",\"price\":%f", price);

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

	mysql_close(con);
	mysql_library_end();

	printf("]\n");
	return 0;
}
