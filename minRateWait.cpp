/* 
 Copyright © Dougie Lawson, 2020-2021, All rights reserved 
*/
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <thread>         // std::this_thread::sleep_until
#include <chrono>         // std::chrono::system_clock
#include <ctime>          // std::time_t, std::tm, std::localtime, std::mktime
#include <csignal>
#include "mysql/mysql.h"
#include "mqttMessage.hpp"
#define DBSERVER "192.168.3.14"
#define DBNAME "mysqluser"
#define DBPASS "mysqlpwd"
#define DBN "Octo"
#define BROKER "192.168.3.14"
#define PORT 1883

using namespace std;
void runATimer(int hour, int min) 
{
	using std::chrono::system_clock;
	std::time_t tt = system_clock::to_time_t (system_clock::now());

	struct std::tm * ptm = std::localtime(&tt);
	//cout << "Current time: " << std::put_time(ptm,"%X") << '\n';

	//cout << "Waiting for "; 
	//cout << setfill('0') << setw(2) << hour << ":"; 
	//cout << setfill('0') << setw(2) << min << endl;

	ptm->tm_hour=hour;ptm->tm_min=min; ptm->tm_sec=0;
	std::this_thread::sleep_until (system_clock::from_time_t (mktime(ptm)));

	return;
}

void sendACommand(string action1, string action2) 
{
	string delimiter = ":";
	string shellies = "shellies/";
	string relay = "/relay/0/command";
	string mqtopic = "";
	string obj1 = action1.substr(0, action1.find(delimiter));
	string act1 = action1.substr(action1.find(delimiter) +1 ,action1.length());

	string obj2 = action2.substr(0, action2.find(delimiter));
	string act2 = action2.substr(action2.find(delimiter) +1 ,action2.length());
	mqtopic += shellies+obj1+relay; 
	mqttMessage shelly1(action1.c_str(), mqtopic.c_str(), BROKER, PORT);
	shelly1.send_message(act1.c_str());
	mqtopic = shellies+obj2+relay; 
	mqttMessage shelly2(action2.c_str(), mqtopic.c_str(), BROKER, PORT);
	shelly2.send_message(act2.c_str());

	return;
}

void signal_handler(int signal)
{
       	sendACommand("pm-1:off", "pm-2:off");
}

int main (int argc, char* argv[])
{

	signal(SIGTERM, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);
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
//	int previousHour = 0;
//	int previousMin = 0;

	if (argc < 2 || argc > 2)
	{
		cerr << "error:" << argc -1 << " arguments passed" << endl;
		return -1;
	}
	if (argc = 2)
	{
		//cout << "number of arguments: " << argc -1 << endl;
		for (int i; i < argc; i++)
		{
			cout << "argv[" << i << "]: " << argv[i] << endl;
		}

		string mP = argv[1];
		maxPrice = stod(mP);
	}

	//cout << "Max price: " << maxPrice << endl;

	const char* sql = "SELECT CONVERT_TZ(period_start, 'UTC', 'Europe/London') as period_start, CONVERT_TZ(period_end, 'UTC', 'Europe/London') as period_end, price FROM Agile_tariffs WHERE period_start >= NOW() LIMIT 1;";

	while (1) {
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
			if (price <= maxPrice) 
			{
				cout << " Price <= maxPrice " << endl;
				sendACommand("pm-1:on", "pm-2:off");
				runATimer(ts_e.hour, ts_e.minute);
			}
			else
			{
				cout << " Price > maxPrice " << endl;
		       		sendACommand("pm-1:off", "pm-2:on");
		       		runATimer(ts_e.hour, ts_e.minute);
			}
		}
		mysql_close(Connection);	
	} 	
}


