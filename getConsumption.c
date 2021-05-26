/* 
 Copyright © Dougie Lawson, 2020-2021, All rights reserved 
*/
#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <curl/curl.h>
#include "json-c/json.h"
#include <mysql.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <pcre.h>
#define COLUMN_COUNT 3
#define FALSE 0
#define TRUE 1 
#include <libconfig.h>
#define CONFIG_FILE "/home/pi/.zappi.cfg"

void sql_commit();
json_object* json_parse(json_object* jObj);
json_object* json_object_parse(json_object* jObj);
json_object* json_parse_array(json_object* jObj, int key);
json_object* print_json_value(json_object* jObj);
int lexer(const char *s);

char* current_key;
char* unk_string;
char nameBuff[24];
FILE* outfile;

enum { unk, cons, from, to, counter };
double consumption;
char* t_from;
char* t_to;
char last_value[100];

struct tm* to_gmt;

MYSQL* con;
MYSQL_STMT *stmt;
int row_count;
const char* d_host;
const char* d_db;
const char* d_user;
const char* d_pwd;
const char* api_key;
const char* elec_mpan;
const char* elec_ser;

int convert(char* timestr)
{
	pcre *regComp;
	pcre_extra *pcreExtra;
	char* aStrRegex = "([0-9]{4})-([0-9]{2})-([0-9]{2})T([0-9]{2}):([0-9]{2}):[0-9]{2}(Z|\\+|-)(([0-9]{2}):([0-9]{2}))?";
	const char* pcreErrorStr;
	int pcreErrorOffset;
	int pcreExecRet;
	int subStrVec[30];
	const char* yearStr;
	const char* monthStr;
	const char* mdayStr;
	const char* hourStr;
	const char* minStr;
	const char* zPlusMinus;
	const char* offHour;
	const char* offMin;
	char chars[30];
	char *token1;
	char *token2;
	char *token3;
	time_t unixtime;
	struct tm not_gmt;
//	struct tm ok_gmt;
	int hour_o, min_o, offset;
	int y, mo, d, h, mi, s;
  	regComp = pcre_compile(aStrRegex, PCRE_CASELESS, &pcreErrorStr, &pcreErrorOffset, NULL);
	if (regComp == NULL) 
	{
		printf("Error: %s %s\n", aStrRegex, pcreErrorStr);
		exit(1);
	}
	pcreExtra = pcre_study(regComp, 0, &pcreErrorStr);
	if (pcreErrorStr != NULL)
	{
		printf("Study: %s %s\n", aStrRegex, pcreErrorStr);
		exit(1);
	}
	pcreExecRet = pcre_exec(regComp, pcreExtra, timestr, strlen(timestr), 0, 0, subStrVec, 30);
	if (pcreExecRet > 0)
	{

		pcre_get_substring(timestr, subStrVec, pcreExecRet, 1, &(yearStr));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 2, &(monthStr));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 3, &(mdayStr));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 4, &(hourStr));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 5, &(minStr));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 6, &(zPlusMinus));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 8, &(offHour));
		pcre_get_substring(timestr, subStrVec, pcreExecRet, 9, &(offMin));

		sscanf(yearStr, "%d", &y);
		sscanf(monthStr, "%d", &mo);
		sscanf(mdayStr, "%d", &d);
		sscanf(hourStr, "%d", &h);
		sscanf(minStr, "%d", &mi);

		not_gmt.tm_year = y - 1900;
		not_gmt.tm_mon = mo - 1;
		not_gmt.tm_mday = d;
		not_gmt.tm_hour = h;
		not_gmt.tm_min = mi;
		not_gmt.tm_sec = 0;

		unixtime = mktime(&not_gmt);

		if (pcreExecRet == 10)
		{	
			sscanf(offHour, "%d", &hour_o);
			sscanf(offMin, "%d", &min_o);
			if (strcmp(zPlusMinus, "+") == 0)
			{
//		printf(" +VE ");
				unixtime -= ((60 * min_o) + (3600 * hour_o));
			}
			else {
//		printf(" -VE ");
				unixtime += ((60 * min_o) + (3600 * hour_o));
			}
//	printf("%04d-%02d-%02d, %02d:%02d %s%02d:%02d  ", y, mo, d, h, mi, zPlusMinus, hour_o, min_o);
		}
//else { 
//	printf("%04d-%02d-%02d, %02d:%02d  ", y, mo, d, h, mi);
//}

		to_gmt = gmtime(&unixtime);
	}
	else if (pcreExecRet < 0)
	{
		switch(pcreExecRet)
		{
			case PCRE_ERROR_NOMATCH : printf("No match\n"); break;
			case PCRE_ERROR_NULL : printf("Null\n"); break;
			case PCRE_ERROR_BADOPTION : printf("Bad opt\n"); break;
			case PCRE_ERROR_BADMAGIC : printf("Voodoo\n"); break;
			case PCRE_ERROR_UNKNOWN_NODE : printf("Something kooky\n"); break;
			case PCRE_ERROR_NOMEMORY : printf("Oom\n"); break;
			default : printf("Very bad\n"); break;
		}
	}
	else
	{
		printf("No error, but no match");
	}

	sprintf(chars, "%s", timestr);	
}

static size_t curl_print(void* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t written =  fwrite(ptr, size, nmemb, (FILE*)stream);
	return written; 
}

char* makeURL ()
{

	time_t t, sT, eT;
	struct tm pf;
	struct tm pt;
	t = time(NULL);
	sT = t - (14*24*60*60); 
	eT = t + (24*60*60);
	pf = *gmtime(&sT);
	pt = *gmtime(&eT);
	char* url = malloc(512);
	sprintf(url, "https://api.octopus.energy/v1/electricity-meter-points/%s/meters/%s/consumption?order_by=period&page_size=30000&period_from=%4d-%02d-%02dT00:00Z&period_to=%4d-%02d-%02dT23:30Z",elec_mpan, elec_ser, pf.tm_year + 1900, pf.tm_mon + 1, pf.tm_mday, pt.tm_year + 1900, pt.tm_mon + 1, pt.tm_mday);

	//printf("%s\n", url);
	return url;
}

void curlGET(void)
{
	CURL* curl;
	CURLcode res;
 
	curl_global_init(CURL_GLOBAL_ALL);
 
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, api_key); 
		curl_easy_setopt(curl, CURLOPT_URL, makeURL());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_print);

		outfile = fopen(nameBuff, "wb");
		if (outfile) {
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);

    			res = curl_easy_perform(curl);
   		 	if(res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			fclose(outfile);
		}
	curl_easy_cleanup(curl);
	}
 
	curl_global_cleanup();
 
}

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

void sql_init()
{
	int rc;
	my_bool trunc = 0;
   	char *sql_delete;
   	char *sql_insert;

	mysql_library_init(0, NULL, NULL);

	con = mysql_init(NULL);
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
 
	stmt = mysql_stmt_init(con);
	if (!stmt) 
	{
		exit_on_error("Stmt init", con);
	}

	sql_insert  = "INSERT IGNORE INTO  Agile_usage ( period_from ,  period_to ,  power_usage ) VALUES( ?, ?, ?)";
	if (mysql_stmt_prepare(stmt, sql_insert, strlen(sql_insert)))
	{
		exit_on_error("Stmt prepare", con);
	}

	row_count = 0;

}

void sql_insert()
{
	int rc;
	int param_count, column_count;
	MYSQL_BIND bind[COLUMN_COUNT];
	MYSQL_TIME ts_start;
	MYSQL_TIME ts_end;
	my_bool error[COLUMN_COUNT];
	my_bool* is_null[COLUMN_COUNT];
	struct tm tm;
	int count = 0;

	param_count = mysql_stmt_param_count(stmt);
	if (param_count != COLUMN_COUNT)
	{
		exit_on_error("Invalid param count", con);
	} 

	memset(bind, 0, sizeof(bind));
	//printf("ts_start ");
	convert(t_from);
	//printf("day: %02d hour: %02d min: %02d \n", to_gmt->tm_mday, to_gmt->tm_hour, to_gmt->tm_min);
	ts_start.year = to_gmt->tm_year + 1900;
  	ts_start.month = to_gmt->tm_mon + 1;
  	ts_start.day = to_gmt->tm_mday;
  	ts_start.hour = to_gmt->tm_hour;
  	ts_start.minute = to_gmt->tm_min;
  	ts_start.second = 0;
  	ts_start.second_part = 0;

	bind[0].buffer_type = MYSQL_TYPE_DATETIME;
	bind[0].buffer = &ts_start;
	is_null[0] = (my_bool)0;
	bind[0].is_null = is_null[0];
	bind[0].error = &error[0];

	//printf("ts_end   ");
  	convert(t_to);
	//printf("day: %02d hour: %02d min: %02d \n", to_gmt->tm_mday, to_gmt->tm_hour, to_gmt->tm_min);
	ts_end.year = to_gmt->tm_year + 1900;
	ts_end.month = to_gmt->tm_mon + 1;
	ts_end.day = to_gmt->tm_mday;
	ts_end.hour = to_gmt->tm_hour;
	ts_end.minute = to_gmt->tm_min;
	ts_end.second = 0;
	ts_end.second_part = 0;

	bind[1].buffer_type = MYSQL_TYPE_DATETIME;
	bind[1].buffer = &ts_end;
	is_null[1] = (my_bool)0;
	bind[1].is_null = is_null[1];
	bind[1].error = &error[1];

	sprintf(last_value, " \"%04d-%02d-%02d %02d:%02d\" - \"%04d-%02d-%02d %02d:%02d\"  %lf \n", ts_start.year, ts_start.month, ts_start.day, ts_start.hour, ts_start.minute, ts_end.year, ts_end.month, ts_end.day, ts_end.hour, ts_end.minute, consumption);


	bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[2].buffer = &consumption;
	is_null[2] = (my_bool)0;
	bind[2].is_null = is_null[2];
	bind[2].error = &error[2];

	if (mysql_stmt_bind_param(stmt, bind))
	{
		exit_on_error("Stmt bind param", con);
	}
	rc = mysql_stmt_execute(stmt);

	if (rc)
	{
		exit_on_error("Stmt execute", con);
	}
	else row_count++;
	if (row_count > 20) sql_commit();

}
void sql_commit()
{
	mysql_commit(con);
	row_count = 0;
}

int sql_terminate()
{
   	if (mysql_stmt_close(stmt))
   	{
        	exit_on_error("Stmt close", con);
   	} 

	mysql_close(con);
	mysql_library_end();
	printf("Last value: %s\n", last_value);
  
	return 0;
}
int lexer(const char *s)
{
	static struct entry_s
	{
		const char *key;
		int token;
	
	}
	token_table[] =
	{
		{ "Unknown", unk },
		{ "consumption", cons },
		{ "interval_start", from },
		{ "interval_end", to },
		{ "count", counter},
	};
	struct entry_s *p = token_table;
	for(; p->key != NULL && strcmp(p->key, s) != 0; ++p);
	return p->token;
}

json_object* print_json_value(json_object* jObj)
{
	json_object* print_result;
	enum json_type type;
	type = json_object_get_type(jObj);
	switch (type)
	{
		case json_type_object:
		case json_type_array:
			printf("json_type_object\n");
			print_result = json_parse(jObj);
			break;
	}
	switch(lexer(current_key)) {
		case counter:
			break;
		case cons:
			consumption = json_object_get_double(jObj);
  			//printf (" Consumption: %f \t", consumption);
			break;
		case from:
			t_from = strdup(json_object_get_string(jObj));
  			//printf (" J_From: %s \t", t_from);
			break;
		case to:
			t_to = strdup(json_object_get_string(jObj));
    			//printf (" J_To: %s \n", t_to);
			break;
		default:
			unk_string = strdup(json_object_get_string(jObj));
			printf ("Unknown JSON key: %s value: %s\n", current_key, unk_string);
	}
	return print_result;
}

json_object* json_parse_array(json_object* jObj, int key)
{
	enum json_type type;
	int i, arraylen, idx;
	json_object* jarray = jObj;
	json_object* jvalue;
	if (key)
	{
		jarray = json_object_array_get_idx(jObj, key);
	}
	type = json_object_get_type(jarray);
	if (type == json_type_array && jarray != NULL)
	{

		arraylen = json_object_array_length(jarray);
	
		for (i=0; i< arraylen; i++)
		{
			jvalue = json_object_array_get_idx(jarray, i);
			type = json_object_get_type(jvalue);
			if (type != json_type_object)
			{
				print_json_value(jvalue);
				json_object_put(jvalue);
			}
			else
			{
				idx = 0;
				json_object_parse(jvalue);
				json_object_put(jvalue);
			}
		}
	}
	return jarray;	
}

json_object* json_object_parse(json_object* jObj)
{
	enum json_type type;
	json_object* parse_result;
	json_object* newObj;
	json_object_object_foreach(jObj, key, val)
	{
		current_key = strdup(key);
		type = json_object_get_type(val);
		switch (type)
		{
			case json_type_boolean: 
			case json_type_double: 
			case json_type_int: 
			case json_type_string:
				print_json_value(val);
				break; 
			case json_type_object:
				newObj = json_tokener_parse(json_object_to_json_string(val));
				parse_result = json_parse(newObj);
				json_object_put(newObj);
				break;
			case json_type_array:
				newObj = json_tokener_parse(json_object_to_json_string(val));
				parse_result = json_parse(newObj);
				break;

		}
	}
	if (parse_result != NULL)
	{
		sql_insert();
	}

	if (parse_result == NULL)
	{
		return parse_result;
	}
}

json_object* json_parse(json_object* jObj)
{
	enum json_type type;
	json_object* parse_result;
	int idx =0;
	parse_result = jObj;
	type = json_object_get_type(jObj);
	do 
	{
		if (parse_result != NULL) 
		{
			switch (type)
			{
				case json_type_boolean: 
				case json_type_double: 
				case json_type_int: 
				case json_type_string:
					print_json_value(jObj);
					break; 
				case json_type_object:
					parse_result = json_object_parse(jObj); 
		  			break;
				case json_type_array:
		  			parse_result = json_parse_array(jObj, idx);
					break;
			}
		}
		idx++;
	}
	while(parse_result != NULL);
	return parse_result;
}

int main(int argc, char* argv[])
{
        config_t cfg;
        config_setting_t *root;
        config_setting_t *database_g, *db_host, *db_db, *db_pwd, *db_user;
	config_setting_t *octopus_g, *oct_api, *oct_mpan, *oct_ser;

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

        octopus_g = config_setting_get_member(root, "Octopus");

        oct_api = config_setting_get_member(octopus_g, "api_key");
        api_key = config_setting_get_string(oct_api);
        oct_mpan = config_setting_get_member(octopus_g, "elec_mpan");
        elec_mpan = config_setting_get_string(oct_mpan);
        oct_ser = config_setting_get_member(octopus_g, "elec_ser");
        elec_ser = config_setting_get_string(oct_ser);

	setenv("TZ", "UTC", 1);
	json_object* tariffJson;

	memset(nameBuff,0,sizeof(nameBuff));
	strncpy(nameBuff, "/tmp/consumptionXXXXXX",23);
	int tempname = mkstemp(nameBuff);
	outfile = fopen(nameBuff, "wb");

	curlGET();
	tariffJson = json_object_from_file(nameBuff);
	sql_init();
	json_parse(tariffJson);
	sql_terminate();
	unlink(nameBuff);

	return 0;
}
