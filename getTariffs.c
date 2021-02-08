#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <curl/curl.h>
#include "json-c/json.h"
#include <mysql.h>
#include <time.h>
#define COLUMN_COUNT 3

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

enum { unk, countj, exVAT, incVAT, from, to };
double ex_VAT;
double inc_VAT;
char* t_from;
char* t_to;

MYSQL* con;
MYSQL_STMT *stmt;
int row_count;

const char* d_host ="192.168.3.14";
const char* d_db = "EV";
const char* d_user = "MySQLUser";
const char* d_pwd = "MySQLPwd";

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
	sT = t - (48*60*60);
	eT = t + (48*60*60);
	pf = *gmtime(&sT);
	pt = *gmtime(&eT);
	char* url = malloc(256);
	sprintf(url, "https://api.octopus.energy/v1/products/AGILE-18-02-21/electricity-tariffs/E-1R-AGILE-18-02-21-H/standard-unit-rates/?page_size=1500&period_from=%4d-%02d-%02dT00:00Z&period_to=%4d-%02d-%02dT23:30Z", pf.tm_year + 1900, pf.tm_mon + 1, pf.tm_mday, pt.tm_year + 1900, pt.tm_mon + 1, pt.tm_mday);

	return url;
}

void curlGET(void)
{
	CURL* curl;
	CURLcode res;
 
	curl_global_init(CURL_GLOBAL_ALL);
 
	curl = curl_easy_init();
	if(curl) {

		curl_easy_setopt(curl, CURLOPT_URL, makeURL());
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

	sql_insert  = "INSERT IGNORE INTO Agile_tariffs(period_start, period_end, price) VALUES( ?, ?, ?)";
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
	memset(&tm, 0, sizeof(struct tm));
	strptime(t_from, "%FT%TZ", &tm);

  	ts_start.year = tm.tm_year + 1900;
  	ts_start.month = tm.tm_mon + 1;
  	ts_start.day = tm.tm_mday;
  	ts_start.hour = tm.tm_hour;
  	ts_start.minute = tm.tm_min;
  	ts_start.second = 0;
  	ts_start.second_part = 0;

	bind[0].buffer_type = MYSQL_TYPE_DATETIME;
	bind[0].buffer = &ts_start;
	is_null[0] = (my_bool)0;
	bind[0].is_null = is_null[0];
	bind[0].error = &error[0];

	memset(&tm, 0, sizeof(struct tm));
	strptime(t_to, "%FT%TZ", &tm);

  	ts_end.year = tm.tm_year + 1900;
  	ts_end.month = tm.tm_mon + 1;
  	ts_end.day = tm.tm_mday;
  	ts_end.hour = tm.tm_hour;
  	ts_end.minute = tm.tm_min;
  	ts_end.second = 0;
  	ts_end.second_part = 0;

	bind[1].buffer_type = MYSQL_TYPE_DATETIME;
	bind[1].buffer = &ts_end;
	is_null[1] = (my_bool)0;
	bind[1].is_null = is_null[1];
	bind[1].error = &error[1];

	bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
	bind[2].buffer = &inc_VAT;
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
		{ "count", countj },
		{ "value_exc_vat", exVAT },
		{ "value_inc_vat", incVAT },
		{ "valid_from", from },
		{ "valid_to", to },
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
		case exVAT:
		case countj:
			//ex_VAT = json_object_get_double(jObj);
			//printf ("Ex VAT: %f ", ex_VAT);
			break;
		case incVAT:
			inc_VAT = json_object_get_double(jObj);
  			//printf (" Inc VAT: %f \t", inc_VAT);
			break;
		case from:
			t_from = strdup(json_object_get_string(jObj));
  			//printf (" From: %s \t", t_from);
			break;
		case to:
			t_to = strdup(json_object_get_string(jObj));
    			//printf (" To: %s \n", t_to);
			break;
		default:
			unk_string = strdup(json_object_get_string(jObj));
//			printf ("Unknown JSON key: %s value: %s\n", current_key, unk_string);
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

	if (parse_result != NULL) sql_insert();

	if (parse_result == NULL) return parse_result;
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
	json_object* tariffJson;

	memset(nameBuff,0,sizeof(nameBuff));
	strncpy(nameBuff, "/tmp/tariffsfil-XXXXXX",23);
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
