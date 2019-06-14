#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <string.h> /* memcpy, memset */
#include <malloc.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library

char* GetPNumber(char*);
char* MessageCreate(char *, char *);
char* modify(char*, long );
void parse(FILE*, char* , long );
char* modifyClassNumber(char* ,long );
void MakeDelay();
long LoadLastNum();
void SaveLastNum(long);

void error(const char *msg) { perror(msg); exit(0); }

int main()
{
	struct hostent *server;
	struct sockaddr_in serv_addr;
	char buff[1025];
	int received;
	int portno = 80;
	char FolderName[40];
	char FileName[40];
	char line[50];
	char *PNumber;
	long offset = LoadLastNum();

	char host[] = "patft.uspto.gov";

	WSADATA wsa;
	SOCKET s;

	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");
	server = gethostbyname(host);
	serv_addr.sin_addr.s_addr = inet_addr(server->h_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(80);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(80);
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

	//_mkdir("E:\\TestsNIS");
	_mkdir("D:\\TestsNIS");

	FILE* PGA = fopen("PGA.txt", "r");
	if (PGA == NULL)
	{
		printf("Couldn't open PGA file...\n");
		return -1;
	}
	else printf("PGA opened....\n");

	fseek(PGA, offset, SEEK_SET);
	char message[256] = { 0 };

	while (!feof(PGA))
	{
		//Create a socket
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			printf("Could not create socket : %d", WSAGetLastError());
		}
		//Connect to remote server
		if (connect(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("connect failed with error code : %d", WSAGetLastError());
			return 1;
		}

		fgets(line, 50, PGA);
		offset += 50;
		PNumber = GetPNumber(line);
		printf("\nPNumber: %s\n", PNumber);
		MessageCreate(PNumber, message);
		//printf("Request:\n%s\n", message);

		if (send(s, message, strlen(message), 0) < 0)
		{
			printf("Send failed with error code : %d", WSAGetLastError());
			return 1;
		}
		//puts("Data Send");

		//Receive a reply from the server
		
		//sprintf(FolderName, "E:\\TestsNIS\\PN\\%s", PNumber);
		sprintf(FolderName, "D:\\TestsNIS\\%s", PNumber);

		_mkdir(FolderName);
		//sprintf(FileName, "E:\\TestsNIS\\PN\\%s\\Origin.txt", PNumber);
		sprintf(FileName, "D:\\TestsNIS\\%s\\Origin.txt", PNumber);

		FILE * Origin = fopen(FileName, "w");
		if (Origin != NULL)
		{
			do {
				received = recv(s, buff, 1024, 0);
				if (received > 0)
				{
					buff[received < 1024 ? received : 1024] = '\0';
					fprintf(Origin, "%s", buff);
				}
				else if (received == 0)
					printf("Connection closed\n");
				else
					printf("recv failed: %d\n", WSAGetLastError());
			} while (received > 0);
			//puts("Reply received");

			//parse
			freopen(FileName, "r", Origin);
			fseek(Origin, 0, SEEK_END);
			long size = ftell(Origin);
			fseek(Origin, 0, SEEK_SET);
			if (size > 0)
			{
				parse(Origin, PNumber, size);
			}
			else printf("FILE is empty\n");
			fclose(Origin);
		}
		else printf("ERROR creating a file\n");
		closesocket(s);
		remove(FileName);
		SaveLastNum(offset);
		MakeDelay();
	}
	//SaveLastNum(offset);
	fclose(PGA);
	WSACleanup();

	return 0;
}

void MakeDelay()
{
	int randNum;
	srand(time(NULL));
	randNum = rand() % 3 + 1;
	printf("Sleep for %d seconds\n", randNum);
	Sleep(randNum * 1000);
}

long LoadLastNum()
{
	long offset = 0; 
	FILE* log = fopen("log.bin", "rb");
	if (log != NULL)
	{
		fread(&offset, sizeof(long), 1, log);
		fclose(log);
	}
	return offset;
}

void SaveLastNum(long offset)
{
	FILE* log = fopen("log.bin", "wb");
	if (log!=NULL)
	{
		fwrite(&offset, sizeof(long), 1, log);
		fclose(log);
		printf("\nNumbers read: %d\n", offset / 50);
	}
	else printf("Couldn't open log.bin file\n");
}

char * MessageCreate(char * PN, char * message)
{
	char host[] = "patft.uspto.gov";
	char path[256] = "netacgi/nph-Parser?Sect1=PTO1&Sect2=HITOFF&d=PALL&p=1&u=%2Fnetahtml%2FPTO%2Fsrchnum.htm&r=1&f=G&l=50&s1=";
	strcat(path, PN);
	strcat(path, ".PN.&OS=PN/");
	strcat(path, PN);
	strcat(path, "&RS=PN/");
	strcat(path, PN);
	sprintf(message, "GET /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/html\r\n\r\n", path, host);

	return message;
}

char* GetPNumber(char* line)
{
	char help[50];
	int i = 0, k = 0;
	int len = strlen(line);

	for (i; i < len; i++)
	{
		if (line[i] == '0')
		{
			i++;
			do
			{
				help[k++] = line[i++];
			} while (line[i] != '\t');
			break;
		}
	}
	help[k] = '\0';
	strncpy(line, help, strlen(help) + 1);
	return line;
}

char* modifyClassNumber(char* string, long size)
{
	char* tmp = (char*)malloc(size + 1);
	long i = 0, j = 0, YoN = 0;
	for (; i < size; i++)
	{
		if (string[i] == '<')
		{
			do {
				i++;
			} while (string[i] != '>');
		}
		else if (string[i] == '&')
		{
			i += 4;
		}
		else if (string[i] == '\0')
			break;
		else tmp[j++] = string[i];
	}
	tmp[j] = '\0';
	return tmp;
}

char* modify(char* string, long size)
{
	char* tmp = (char*)malloc(size + 1);
	long i = 0, j = 0, YoN = 0;
	for (; i < size; i++)
	{
		if (string[i] == '<')
		{
			do {
				i++;
			} while (string[i] != '>');
		}
		else if (string[i] == '\0')
			break;
		else tmp[j++] = string[i];
	}
	tmp[j] = '\0';
	return tmp;
}

void parse(FILE* origin, char* PN, long size)
{
	char abstractName[40];
	char numbersName[40];
	char claimsName[40];
	char descriptionName[40];
	/*sprintf(abstractName, "E:\\TestsNIS\\PN\\%s\\Abstract.txt", PN);
	sprintf(numbersName, "E:\\TestsNIS\\PN\\%s\\Class number.txt", PN);
	sprintf(claimsName, "E:\\TestsNIS\\PN\\%s\\Claims.txt", PN);
	sprintf(descriptionName, "E:\\TestsNIS\\PN\\%s\\Description.txt", PN);*/

	sprintf(abstractName, "D:\\TestsNIS\\%s\\Abstract.txt", PN);
	sprintf(numbersName, "D:\\TestsNIS\\%s\\Class number.txt", PN);
	sprintf(claimsName, "D:\\TestsNIS\\%s\\Claims.txt", PN);
	sprintf(descriptionName, "D:\\TestsNIS\\%s\\Description.txt", PN);

	FILE * abstract = fopen(abstractName, "w");
	FILE * numbers = fopen(numbersName, "w");
	FILE * claims = fopen(claimsName, "w");
	FILE * description = fopen(descriptionName, "w");
	if ((abstract != NULL) && (numbers != NULL) && (claims != NULL) && (description != NULL))
	{
		char* html = (char*)malloc(size + 1);
		if (html != NULL)
		{
			size_t nread = fread(html, 1, size, origin);

			//FINDING abstract part
			char* abstr = strstr(html, "<BR><CENTER><b>Abstract</b></CENTER>");
			if (abstr != NULL)
			{
				char* beg = strstr(abstr, "<p>");
				char* end = strstr(beg, "</p>");
				long beglen = strlen(beg);
				long endlen = strlen(end);
				long bodylen = beglen - endlen;
				char *strBODY = (char*)malloc(bodylen + 1);
				memcpy(strBODY, beg, bodylen);
				fprintf(abstract, "%s", modify(strBODY, bodylen));
				free(strBODY);
			}

			//FINDING class number
			char* clss = strstr(html, "<b>Current International Class: </b>");
			if (clss != NULL)
			{
				char* beg = strstr(clss, "</TD>");
				char* end = strstr(beg, "</TR>");
				long beglen = strlen(beg);
				long endlen = strlen(end);
				long bodylen = beglen - endlen;
				char *strBODY = (char*)malloc(bodylen + 1);
				memcpy(strBODY, beg, bodylen);
				fprintf(numbers, "%s", modifyClassNumber(strBODY, bodylen));
				free(strBODY);
			}

			//FINDING claims
			char* clms = strstr(html, "<CENTER><b><i>Claims</b></i></CENTER>");
			if (clms != NULL)
			{
				char* beg = strstr(clms, "<BR><BR>");
				char* end = strstr(beg, "<HR>");
				long beglen = strlen(beg);
				long endlen = strlen(end);
				long bodylen = beglen - endlen;
				char *strBODY = (char*)malloc(bodylen + 1);
				memcpy(strBODY, beg, bodylen);
				fprintf(claims, "%s", modify(strBODY, bodylen));
				free(strBODY);
			}

			//FINDING description
			char* dscrp = strstr(html, "<CENTER><b><i>Description</b></i></CENTER>");
			if (dscrp != NULL)
			{
				char* beg = strstr(dscrp, "<BR><BR>");
				char* end = strstr(beg, "<HR>");
				long beglen = strlen(beg);
				long endlen = strlen(end);
				long bodylen = beglen - endlen;
				char *strBODY = (char*)malloc(bodylen + 1);
				memcpy(strBODY, beg, bodylen);
				fprintf(description, "%s", modify(strBODY, bodylen));
				free(strBODY);
			}

			free(html);
		}
		else printf("can't allocate mem for html file\n");
		fclose(abstract), fclose(numbers), fclose(claims), fclose(description);
	}
	else printf("ERROR 1\n");
}
