#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <winsock2.h>
#include <windows.h>  
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 25565

typedef struct {
    char id[10];
    char password[20];
} userinf;

typedef struct {
    int car;
    int elec;
    int disposable;
    double total;
    struct tm today;
} userdat;

DWORD WINAPI ClientHandler(void* clientSocket);

int main()
{
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in server, client;
    int c;

    // Winsock 초기화
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 소켓 생성
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 서버 주소 설정
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // 바인딩
    bind(serverSocket, (struct sockaddr*)&server, sizeof(server));

    // 대기
    listen(serverSocket, 5);

    printf("서버가 실행 중입니다...\n");

    c = sizeof(struct sockaddr_in);
    while (1) 
    {
        clientSocket = accept(serverSocket, (struct sockaddr*)&client, &c);
        if (clientSocket == INVALID_SOCKET) {
            printf("클라이언트 연결 실패\n");
            continue;
        }
        printf("클라이언트 연결됨\n");

        // 쓰레드 생성
        CreateThread(NULL, 0, ClientHandler, (void*)clientSocket, 0, NULL);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void today(struct tm* t)
{
    time_t now = time(NULL);
    localtime_s(t, &now);

    t->tm_year += 1900;
    t->tm_mon += 1;
}

void data_store(userdat* data, const char* id)
{
    int cnt;
    char filename[20];
    snprintf(filename, sizeof(filename), "%s.dat", id);

    FILE* fp = fopen(filename, "rb+");
    if (fp == NULL)
    {
        fp = fopen(filename, "wb+");
        if (fp == NULL)
        {
            perror("파일 열기 실패");
            return;
        }
        cnt = 1;
    }
    else
    {
        if (fread(&cnt, sizeof(int), 1, fp) != 1)
        {
            cnt = 1;
        }
        else
        {
            cnt++;
        }
    }

    fseek(fp, 0, SEEK_SET);
    fwrite(&cnt, sizeof(int), 1, fp);

    fseek(fp, 0, SEEK_END);
    if (fwrite(data, sizeof(userdat), 1, fp) != 1) {
        perror("데이터 저장 실패");
    }

    fclose(fp);
}

userdat* Import_Data(const char* id, int* count)
{
    int cnt;
    char filename[20];
    snprintf(filename, sizeof(filename), "%s.dat", id);

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("파일 열기 실패\n");
        return NULL;
    }
    fread(&cnt, sizeof(int), 1, fp);

    userdat* data = (userdat*)malloc(sizeof(userdat) * cnt);

    if (data == NULL) {
        printf("메모리 할당 실패\n");
        fclose(fp);
        return NULL;
    }
    fread(data, sizeof(userdat), cnt, fp);
    //printf("cnt: %d\n", cnt);
    *count = cnt;
    fclose(fp);
    return data;
}

int login_process(userinf* user)
{
    userinf userdata;
    FILE* fp = fopen("userdat.dat", "rb+");
    if (fp == NULL)
    {
        return-1;
    }
    while (fread(&userdata, sizeof(userdata), 1, fp) == 1)
    {
        if (strcmp(userdata.id, user->id) == 0 && strcmp(userdata.password, user->password) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return -1;
}

void signin(userinf user)
{
    FILE* fp = fopen("userdat.dat", "ab+");
    fwrite(&user, sizeof(userinf), 1, fp);
    fclose(fp);
}

DWORD WINAPI ClientHandler(void* clientSocket) {  
   SOCKET sock = (SOCKET)clientSocket;  
   userinf user;  
   char op;  
   int cnt;  
   userdat* data = NULL; // Initialize the pointer to NULL  

   while ((recv(sock, &op, sizeof(char), 0)) > 0)  
   {  
	   printf("op: %c\n", op);
       if (op == 'l')
       {  
		   printf("Login\n");
           recv(sock, (char*)&user, sizeof(user), 0);  

           printf("%s\n", user.id);  
           printf("%s\n", user.password);  

           // Echo back  
           if (login_process(&user) == 1)  
           {  
               send(sock, "s", 1, 0); 
           }  
           else  
           {  
               send(sock, "f", 1, 0);  
           }  
       }  
       else if (op == 's')
       {  
		   printf("Sign in\n");
           recv(sock, (char*)&user, sizeof(user), 0);  
           signin(user);  
       }  
       else if (op == 'a')
       {  
		   printf("Add\n");
           data = (userdat*)malloc(sizeof(userdat));   
           if (data == NULL)  
           {  
               perror("메모리 할당 실패");  
               break;  
           }  
           recv(sock, (char*)data, sizeof(userdat), 0);  
           today(&(data->today));
           data_store(data, user.id);  
           free(data);   
           data = NULL;  
       }  
       else if (op == 'i')
       {  
		   printf("Import\n");
           data = Import_Data(user.id, &cnt);  
           if (data != NULL)  
           {  
               send(sock, (char*)&cnt, sizeof(cnt), 0);  
               send(sock, (char*)data, sizeof(userdat) * cnt, 0);  
               free(data);  
               data = NULL;  
           }  
		   else if (data == NULL)
		   {
			   cnt = 0;
			   send(sock, (char*)&cnt, sizeof(cnt), 0);
		   }
       }  
   }  

   printf("클라이언트 연결 종료\n");  
   closesocket(sock);  
   return 0;  
}