// (C) Copyright 2020,2021, Dougie Lawson. All rights reserved.
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#define __USE_XOPEN
#include <time.h>
#define DATABASE "/home/pi/octopus/tariffs.db"

char *comma = " ";

static void fetchResults(sqlite3_stmt* stmt)
{
   int i;
   const unsigned char* dateTime;
   char backgroundColour[40];
   char density[20];
//   char* token1, *token2, *token3, *token4, *token5;
//   char* stringp = dateTime;
   //float price;
   const char* ddash = "-";
   const char* dspace = " ";
   const char* dcolon = ":";
   time_t now = time(NULL);
   struct tm* localNow = localtime(&now);
   struct tm ltm = {0};
   int nowHH;

//Content-Type: application/json
//
// [
//  {"periodStart":"18-10 07:00"
// , "price":11.2455}
// ]

   printf("%s{", comma);
        // Date & time format is yyyy-mm-dd hh:mm
        // Split that into 
        // Date (token1): mm-dd
        // Time (token2): hh:mm
        dateTime = sqlite3_column_text(stmt, 0);
	strptime(dateTime, "%Y-%m-%d %H:%M", &ltm);
//	sprintf(token1, "%d", ltm.tm_year+1900);
//	sprintf(token2, "%d", ltm.tm_mon+1);
//	sprintf(token3, "%d", ltm.tm_mday);
//	sprintf(token4, "%d", ltm.tm_hour);
//	sprintf(token5, "%d", ltm.tm_min);
	
//        token1 = strsep(&stringp, ddash);
//       token2 = strsep(&stringp, ddash);
//        token3 = strsep(&stringp, dspace);
//        token4 = strsep(&stringp, dcolon);
//        token5 = strsep(&stringp, dcolon);

	nowHH = localNow->tm_min> 29 ? 30 : 0;
	if (ltm.tm_hour == localNow->tm_hour && ltm.tm_min == nowHH)
	{
		sprintf(density,"1.0");
	}
	else 
	{
		sprintf(density,"0.2");
	}
        
        printf("\"periodStart\":\"%02d-%02d %02d:%02d\"", ltm.tm_mon, ltm.tm_mday, ltm.tm_hour, ltm.tm_min); 
	double priceF = sqlite3_column_double(stmt, 1);
        printf(",\"price\":%.2f", priceF);
	if (priceF > 25.0)
	{
		//                      dark red     
		sprintf(backgroundColour,"167 ,22 ,23");
	}
	else if (priceF > 20.0) 
	{
		//                      red     
		sprintf(backgroundColour,"255 ,0 ,21");
        }
       	else if (priceF > 15.0)
       	{
		//                      light red     
		sprintf(backgroundColour,"250 ,116 ,97");
	}
       	else if (priceF > 10.0)
       	{
		//                      dark green     
		sprintf(backgroundColour,"0 ,100 ,0");

	}
       	else if (priceF > 5.0)
	{
		//                      green     
		sprintf(backgroundColour,"61 ,186 ,20");
	}
	else if (priceF > 0.0)
	{
		//                      light green      
		sprintf(backgroundColour,"144 ,238 ,144");
	}
	else if (priceF > -5.0)
	{
		//                      light blue     
		sprintf(backgroundColour,"197 ,208 ,232");
	}
	else if (priceF > -10.0)
	{
		//                      blue     
		sprintf(backgroundColour,"0 ,18 ,255");
	}
	else
	{
		//                      dark blue     
		sprintf(backgroundColour,"0 ,0 ,139");
	}
	printf(",\"backgroundColor\":\"rgba(%s, %s)\"", backgroundColour, density);
	printf(",\"borderColor\":\"rgb(%s)\"",backgroundColour);
      
      comma=",";
   
   printf("}\n");
}

int main(int argc, char* argv[])
{
   sqlite3* db;
   sqlite3_stmt* stmt;
   char* zErrMsg = 0;
   int rc;
   char* sql;
   
   printf("Content-Type: application/json\r\n\r\n[\n");
   setenv("TZ", "Europe/London", 1);

   rc = sqlite3_open(DATABASE, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }
   rc = sqlite3_exec(db, "PRAGMA query-only", NULL, NULL, &zErrMsg);

   sql = "SELECT datetime(period_start, \'localtime\') as period_start, price FROM agile_tariffs WHERE period_start between datetime('now', 'localtime', '-12 hours') and datetime('now', 'localtime', '+12 hours') ORDER BY period_start;";

   sqlite3_prepare_v3(db, sql, strlen(sql)+1, 0, &stmt, NULL);
   while (1)
   {
	   int s;
	   s = sqlite3_step(stmt);
	   if (s == SQLITE_ROW)
	   {
		   fetchResults(stmt);
	   }
	   else if (s == SQLITE_DONE)
	   {
		   break;
	   }
	   else
	   {
		   fprintf(stderr, "sqlite_step failed\n");
		   exit(1);
	   }
   }
   printf("]\n");
   sqlite3_clear_bindings(stmt);
   sqlite3_reset(stmt);
   sqlite3_close(db);
   return 0;
}
