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

using namespace std;
void Configuration();
void ClearScreen();
void* MonitoraSerial(void*);

pthread_t thread1;
int fd;
char *ret_cfg_rs232;

void handle_sigint(int sig) {
	printf("Caught signal %d\n", sig);
	pthread_cancel(thread1);
	exit(1);
}

int main() {
	ClearScreen();
	signal(SIGINT, handle_sigint);

	if (pthread_create(&thread1, NULL, &MonitoraSerial, NULL) != 0) {
		perror("pthread_create() error");
		exit(1);
	}

	int option = 0;
	string menu;
	do {
		ClearScreen();
		cout << "--------------------------------------------------" << endl;
		cout << "------------------ LAB02 - TASE ------------------" << endl;
		cout << "-----------------------MENU-----------------------" << endl;
		cout << "- \t 1 - Configurar Parametros               -" << endl;
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

	string serial_name, boudrate, tx_amostra;
	cout << "--------------------------------------------------" << endl;
	cout << "------------------CONFIGURACAO--------------------" << endl;
	cout << "- \t 1 - Entre com name da Serial: ";
	std::getline(cin, serial_name);
	cout << "- \t 2 - Entre com Boudrate: ";
	std::getline(cin, boudrate);
	cout << "- \t 2 - Entre com a taxa de amostragem: ";
	std::getline(cin, tx_amostra);
	cout << "--------------------------------------------------" << endl;
	if (pthread_join(thread1, NULL) != 0) {
		perror("pthread_create() error");
		exit(3);
	}
	for (int i = 1; i <= 10; i++) {
		delay(1000);
		cout << "\r-- Aguardando retorno - : " << i << " segundos"
				<< std::flush;
	}
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
	printf("Hello i'm thread ... \n");
	while (1) {
		delay(1);
		while (serialDataAvail(fd)) {
			printf(" -> %3d", serialGetchar(fd));
			fflush(stdout);
		}
	}
	pthread_exit(NULL);
}
