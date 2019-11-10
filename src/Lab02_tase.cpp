//============================================================================
// Name        : Lab02_tase.cpp
// Author      : @vrjesus
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <errno.h>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>

using namespace std;
#define GREEN   "\033[32m"
#define RESET   "\033[0m"

void Configuration();
void ClearScreen();
const std::string currentDateTime();
void* MonitoraSerial(void*);
void* ProcessMsg(void *args);
sqlite3* OpenOrCreate(string database_name);
void CloseDatabase(sqlite3 *DB);
inline bool database_exists(const std::string &filename);
void SaveDataInDatabase(const char *led, const char *ad, const char *time, const char *sName, const char *bdr,
		const char *tx);

pthread_t thr_serial, process_msg;
int fd;
string serial_msg_received = "", serial_config_msg = "      ", database_name =
		"Lab02_tase.db";
string serial_name, boudrate, tx_amostra, led_limite;

void handle_sigint(int sig) {
	printf("Exit APP %d\n", sig);
	pthread_cancel(thr_serial);
	exit(1);
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int main(int argc, char **argv) {
	ClearScreen();
	signal(SIGINT, handle_sigint);
	int option = 0;
	string menu;
	do {
		ClearScreen();
		cout << "--------------------------------------------------" << endl;
		cout << "------------------ LAB02 - TASE ------------------" << endl;
		cout << "-----------------------MENU-----------------------" << endl;
		cout << "- \t 1 - Configurar Parametros\t" << GREEN << serial_config_msg
				<< RESET << "   -" << endl;
		cout << "- \t 2 - Relatorio Database                  -" << endl;
		cout << "- \t 3 - SAIR                                -" << endl;
		cout << "--------------------------------------------------" << endl;
		cout << "\t Opcao:";
		std::getline(cin, menu);
		option = std::atoi(menu.c_str());

		switch (option) {
		case 1:
			ClearScreen();
			Configuration();
			break;
		case 2:
			break;
		case 3:
			option = -1;
			break;
		}

	} while (option >= 0);

	return 0;
}


void Configuration() {

	cout << "--------------------------------------------------" << endl;
	cout << "------------------CONFIGURACAO--------------------" << endl;
	cout << "- \t 1 - Entre com name da Serial: ";
	std::getline(cin, serial_name);
	cout << "- \t 2 - Entre com Boudrate: ";
	std::getline(cin, boudrate);
	cout << "- \t 3 - Entre com a taxa de amostragem (segundos): ";
	std::getline(cin, tx_amostra);
	cout << "- \t 4 - Entre com o minimo para acender LED: ";
	std::getline(cin, led_limite);
	cout << "--------------------------------------------------" << endl;

	cout << "\r--Aguarde ... iniciando configuração" << std::flush;
	if ((fd = serialOpen(("/dev/" + serial_name).c_str(),
			std::atoi(boudrate.c_str()))) < 0) {
		fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
		return;
	} else {
		string msg;
		msg += tx_amostra;
		msg += ";";
		msg += led_limite;
		fflush(stdout);
		serialPuts(fd, msg.c_str());
	}

	pthread_create(&thr_serial, NULL, MonitoraSerial, NULL);

	for (int i = 1; i <= 10; i++) {
		delay(1000);
		cout << "\r-- Aguardando retorno - : " << i << " segundos"
				<< std::flush;
		if (strcmp("cfg_ok", serial_config_msg.c_str()) == 0) {
			//configuração ok
			cout << "\r-- Configuração realiza com Sucesso ... :" << i
					<< " segundos" << std::flush;
			delay(1000);

			return;
		}
	}
	//
	cout << "\n--------------------------------------------------" << std::flush
			<< endl;
}

void ClearScreen() {
	/*if (system("CLS"))
	 system("clear");
	 */
	std::system("clear");
}

void* MonitoraSerial(void*) {
	while (1) {
		while (serialDataAvail(fd)) {
			int rcv = serialGetchar(fd);
			if (rcv > 31) {
				serial_msg_received += (char) rcv;
			} else {
				if ((char) serial_msg_received[0] == '@') {
					serial_msg_received.erase(0, 1);
					serial_config_msg = serial_msg_received;
					serial_msg_received = "";
				} else {
					//call thread process msg
					char *data = new char[serial_msg_received.size() + 1];
					copy(serial_msg_received.begin(), serial_msg_received.end(),
							data);
					data[serial_msg_received.size()] = '\0';

					cout << "Thread new MSG-> " << serial_msg_received << endl;
					pthread_create(&process_msg, NULL, &ProcessMsg,
							(void*) data);
					serial_msg_received = "";
				}

			}
			fflush(stdout);
		}
	}
	pthread_exit(NULL);
}

void* ProcessMsg(void *args) {
	char *msg = (char*) args;

	char *status_led;
	char *ad_value;

	const char delimiter[2] = ";";
	status_led = strtok(msg, delimiter);
	ad_value = strtok(NULL, delimiter);
	SaveDataInDatabase(status_led, ad_value, currentDateTime().c_str(), serial_name.c_str(), boudrate.c_str(), tx_amostra.c_str());
	pthread_exit(NULL);
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

	return buf;
}

sqlite3* OpenOrCreate(string database_name) {
	sqlite3 *DB;
	char *sql;
	int rc;
	char *zErrMsg = 0;

	if (database_exists(database_name)) {
		int exit = 0;
		exit = sqlite3_open(database_name.c_str(), &DB);

		if (exit) {
			std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
			return NULL;
		} else
			std::cout << "Opened Database Successfully!" << std::endl;
		return DB;
	} else {
		int exit = 0;
		exit = sqlite3_open(database_name.c_str(), &DB);

		if (exit) {
			std::cerr << "Error open DB " << sqlite3_errmsg(DB) << std::endl;
			return NULL;
		} else {
			std::cout << "Opened Database Successfully!" << std::endl;
			/* Create SQL statement */
			sql = "CREATE TABLE DEVICE("
					"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
					"STATUS CHAR(10) NOT NULL,"
					"AD_VALUE CHAR(10) NOT NULL,"
					"TIME CHAR(80) NOT NULL,"
					"S_NAME CHAR(10) NOT NULL,"
					"BOUDRATE CHAR(10) NOT NULL,"
					"AMOSTRAGEM CHAR(10) NOT NULL);";

			/* Execute SQL statement */
			rc = sqlite3_exec(DB, sql, callback, 0, &zErrMsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			} else {
				fprintf(stdout, "Table created successfully\n");
			}
		}
		return DB;
	}
}

void CloseDatabase(sqlite3 *DB) {
	if (DB != NULL) {
		sqlite3_close(DB);
		printf("FECHOUUUUU");
	}
}

inline bool database_exists(const std::string &filename) {
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

void SaveDataInDatabase(const char *led, const char *ad, const char *time, const char *sName, const char *bdr,
		const char *tx) {
	char *zErrMsg = 0;
	int rc;
	char *sql;
	sqlite3 *DB = OpenOrCreate(database_name);
	printf("CRIOUUUUU");
	sql = sqlite3_mprintf("INSERT INTO DEVICE (STATUS, AD_VALUE, TIME, S_NAME, BOUDRATE, AMOSTRAGEM) VALUES ('%s', '%s', '%s', '%s', '%s', '%s');", led, ad, time,sName, bdr, tx);
	//sql ="INSERT INTO DEVICE (STATUS, AD_VALUE, TIME, S_NAME, BOUDRATE, AMOSTRAGEM) VALUES ('1234', '123','123','123','123','123');";
	/*sql ="INSERT INTO DEVICE (STATUS, AD_VALUE, TIME, S_NAME, BOUDRATE, AMOSTRAGEM) VALUES (";
	strcat(sql, "'");
	strcat(sql, led);
	strcat(sql, "'");
	strcat(sql, ",");
	strcat(sql, "'");
	strcat(sql, ad);
	strcat(sql, "'");
	strcat(sql, ",");
	strcat(sql, "'");
	strcat(sql, time);
	strcat(sql, "'");
	strcat(sql, ",");
	strcat(sql, "'");
	strcat(sql, sName);
	strcat(sql, "'");
	strcat(sql, ",");
	strcat(sql, "'");
	strcat(sql, bdr);
	strcat(sql, "'");
	strcat(sql, ",");
	strcat(sql, "'");
	strcat(sql, tx);
	strcat(sql, "'");
	strcat(sql, ");");
	printf("CONCATENOUUUUU");
	*/
	rc = sqlite3_exec(DB, sql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		fprintf(stdout, "Table ADDdddd successfully\n");
	}
	CloseDatabase(DB);
}
