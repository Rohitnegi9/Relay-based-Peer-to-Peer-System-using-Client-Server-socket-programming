#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define TEMPFILENAME "8876577559"
#define DOWNLOADFILECOPY "copy.txt"
#define BACKLOG 5

struct PeerNodes
{
    char address[64];
    int port;
};

struct Container
{
    char* name;
    char* value;
};

char * getfieldvalue(struct Container * bundle ,char *field);   
struct Container * packet_parser(char *buffer);
int SendAck(int clientSock,char *prefix);
int checkack(int clientSock);
int KeepFile(int clientSock,const char * filename);
int KeepFile1(int clientSock,char* filename);
int ObtainFile(int clientSock,char * filename);

struct PeerNodes* getnodeslist(int clientSock){
    char c, port[16],address[64];
    int num, turn,recvMsgSize,sendMsgSize, count = 1;
    
    struct PeerNodes* nodeslist = (struct PeerNodes*)malloc(sizeof(struct PeerNodes)*128);
    num = ObtainFile(clientSock,TEMPFILENAME);
    if(num < 0){
        printf("ObtainFile() error while fetching peer nodes list");
        exit(EXIT_FAILURE);
    }
    
    FILE* fp = fopen(TEMPFILENAME,"r");
    c = getc(fp);
    turn = 20;
    for(int i=1;c!=EOF;i++){
        ungetc(c,fp);

        fscanf(fp,"%s",address);
        fscanf(fp,"%s",port);
        strcpy(nodeslist[count].address,address);
        nodeslist[count++].port = atoi(port);
        c = getc(fp);
        i--;
    }
    
    if(!turn)
    printf("Node is not accessible");
    remove(TEMPFILENAME);
    memset(nodeslist[0].address,'\0',sizeof(nodeslist[0].address));
    nodeslist[0].port = count;
    return nodeslist;
}

int ObtainFile_(int clientSock,char * filename);

int Connection_status(int num , char * servIP)
{
    if(num>=0)
   {
      printf("Successfully connected to server with address %s\n",servIP);
      return 1;
      }
      
      return 0;
    }


void clientUtil(struct PeerNodes*);
int connectnode(char* servIP,int servPort,char* filename){
    
    struct sockaddr_in servAddr;
    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSock  < 0)
    return -1;
    
    memset(&servAddr, '\0', sizeof(servAddr)); 

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servPort);
    servAddr.sin_addr.s_addr = inet_addr(servIP);

    int recvMsgSize,sendMsgSize, num = connect(clientSock, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if( num < 0){
        close(clientSock);
        return -1;
    }
    else{
        printf("Connected to peer %s:%d\n",servIP,servPort);
    }

    char buffer[1024];
    
    
    strcpy(buffer,"request:client");
    num = sendMsgSize = write(clientSock,buffer,strlen(buffer));
    if(num < 0){
        close(clientSock);
        return -1;
    }

    num = checkack(clientSock);
    if(num < 0){
        close(clientSock);
        return -1;
    }

    sprintf(buffer,"request:client\nfilename:%s",filename);
    sendMsgSize = write(clientSock,buffer,strlen(buffer));
    if(sendMsgSize < 0){
        close(clientSock);
        return -1;
    }
    
    num = checkack(clientSock);
    if(num < 0){
        close(clientSock);
        return -1;
    }

    memset(buffer,'\0',sizeof(buffer));
    recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1);
    if(recvMsgSize < 0){
        close(clientSock);
        return -1;
    }
    
    SendAck(clientSock,"client");


    if(strcmp(getfieldvalue(packet_parser(buffer),"file"),"yes") ==0){
        if(ObtainFile_(clientSock,TEMPFILENAME) < 0){
            close(clientSock);
            return -1;
        }
        FILE* fp1 = fopen(TEMPFILENAME,"r");
        FILE* fp2 = fopen(DOWNLOADFILECOPY,"w");
        char c;
        c = getc(fp1);
        for(int i=1; c!=EOF; i++)
        {
            ungetc(c,fp1);
            memset(buffer,'\0',sizeof(buffer));
            fgets(buffer,sizeof(buffer)-1,fp1);
            printf("%s", buffer);
            fprintf(fp2,"%s\n",buffer);
            c = getc(fp1);
            i++;
        }
        
        printf("\n");
        fclose(fp1);
        fclose(fp2);
        printf("Fetched file in %s\n", DOWNLOADFILECOPY);
        remove(TEMPFILENAME);
        printf("Closing connection with peer %s:%d gracefully\n", servIP,servPort);
        shutdown(clientSock,0);
        return 0;
    }
    else{
        // printf("File '%s' not found on peer %s:%d\n",filename,servIP,servPort);
        printf("Closing connection with peer %s:%d gracefully\n", servIP,servPort);
        shutdown(clientSock,0);
        return -1;
    }   
}

int main(int argc,char * argv[])
{
    if(argc!=3){
        printf("Wrong input, <Executable code> <Server IP Address> <Server Port number>");
        exit(EXIT_FAILURE);
    }
    // Number of input should be 3 only, otherwise it will be error
    
    char* servIP = argv[1];
    int servPort = atoi(argv[2]);
    int num; 
    
    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSock < 0){
    	printf("Error in the socket()");
        exit(EXIT_FAILURE);
    } 

    struct sockaddr_in servAddr;
    memset(&servAddr, '\0', sizeof(servAddr)); 

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servPort);
    servAddr.sin_addr.s_addr = inet_addr(servIP);

    num = connect(clientSock, (struct sockaddr *)&servAddr, sizeof(servAddr)); 
    
    num = Connection_status(num, servIP);
    if( num == 0){
       printf("connect() failed");
       exit(EXIT_FAILURE);
    }

    char Buffer[1024];
    
    strcpy(Buffer,"request:client");
    int sendMsgSize = write(clientSock,Buffer,strlen(Buffer));
    if( sendMsgSize < 0){
    	printf(" write() error ");
        exit(EXIT_FAILURE);
    }
    

    memset(Buffer,'\0',sizeof(Buffer));
    
    int recvMsgSize = read(clientSock,Buffer,sizeof(Buffer) - 1);
    if( recvMsgSize < 0){
    	printf(" read() error ");
        exit(EXIT_FAILURE);
    }
    
    SendAck(clientSock,"client");

    struct Container *packet = packet_parser(Buffer);
    printf(" Peer nodes list fetched from the server\n");

    num = strcmp(getfieldvalue(packet,"status"),"connected"); 
    if(num !=0){
        printf("Server gave unexpected response ");
        exit(EXIT_FAILURE);
    }
    
    struct PeerNodes * nodeslist = getnodeslist(clientSock);
    
    num = shutdown(clientSock,0);
    if(num< 0){
        printf("shutdown() error closing connection with server.");
        exit(EXIT_FAILURE);
    }
    
    else
    printf("Connection with server closed gracefully\n");
    
    
    char c[10] = "y";
    
    for(int i=1; i<10; i++){
        
        if(strcmp("Y",c)==0 || strcmp("y",c)==0){
            clientUtil(nodeslist);
        }
        else if(strcmp("N",c)==0 || strcmp("n",c)==0){
            printf("Client exiting......\n");    
            i=20;
        }
        else{
            printf("Unknown option\n");
        }
        printf("Press N or n to quit or Y or y to continue\n");
        scanf("%s",c);
        i--;
    }

    return 0;
}

void clientUtil(struct PeerNodes * nodeslist){

    int turn,num_nodes,i=1;

    char filename[64];
    printf("File to download : ");
    scanf("%s",filename);
    num_nodes=nodeslist[0].port;

    int flag = 0;
    while(i<num_nodes-1)
    {
        if(connectnode(nodeslist[i].address,nodeslist[i].port,filename) == 0){
            flag = 1;
            break;
        }
        i++;
    }

    if(flag == 1){
        printf("File '%s' found on peer %s:%d\n",filename,nodeslist[i].address,nodeslist[i].port);
    }
    else{
        printf("File '%s' not found on any peer node\n",filename);
    }
    
    return;
}



struct Container * packet_parser(char *buffer){
    struct Container* packet_fields = (struct Container*)malloc(sizeof(struct Container)*32);
    int fields_num = 1;
    char* temp = strtok(buffer,"\n");
    int sock_num=10;
    while(temp){
        packet_fields[fields_num++].name = temp;
        temp = strtok(NULL,"\n");
    }
    
    if(sock_num==11)
    printf("No field value exist");

    char* fields_num_string = (char*)malloc(sizeof(char)*32);
    sprintf(fields_num_string,"%d",fields_num);

    
    packet_fields[0].value = fields_num_string;
    packet_fields[0].name = NULL;
    
    int i=1;
    while(i<fields_num)
        {
        temp = strtok(packet_fields[i].name,":");
        packet_fields[i].name = temp;
        temp = strtok(NULL,":");
        packet_fields[i].value = temp;
        i++;
    }

    return packet_fields;
}

int checkack(int clientSock){
    
    int num,MsgSize;
    char buffer[1024];
    memset(buffer,'\0',sizeof(buffer));
    MsgSize = read(clientSock,buffer,sizeof(buffer) - 1);
    if(MsgSize < 0)
        return -1;
    num =  strcmp(getfieldvalue(packet_parser(buffer),"ack"),"1"); 
    if(num != 0)
    return -1;
    return 0;
}

char * getfieldvalue(struct Container * bundle ,char *field){
    int n;
    n = atoi(bundle[0].value);
    while(n>0 && strcmp(bundle[--n].name,field)!=0);
    if(n<=0){
        return NULL;
    }
    return bundle[n].value;
}

int SendAck(int clientSock,char *prefix){
    
    int num , sendMsgSize;
    char buffer[1024];
    num = strcmp(prefix,"client");
    if(num == 0) 
    strcpy(buffer,"response:client\nack:1");
    else
    strcpy(buffer,"response:node\nack:1");
    sendMsgSize = write(clientSock,buffer,strlen(buffer));
    if(sendMsgSize < 0)
    {
        if(strcmp(prefix,"node") == 0)
            printf("request write() ack error");
        else
            printf("write() ack send error");
        exit(EXIT_FAILURE);
    }
}

int KeepFile(int clientSock,const char * filename){
    char buffer[1024],response[128];
    int fd, recvMsgSize, sendMsgSize;
    
    struct stat file_stat;
    fd = open(filename,O_RDONLY);
    if(fd < 0){
        strcpy(response,"response:node\nfile:#####");
        sendMsgSize = write(clientSock,response,strlen(response));
        if(sendMsgSize < 0){
            printf("Unable to send node server response\n");
            exit(EXIT_FAILURE);
        }
        recvMsgSize = checkack(clientSock);
        if(recvMsgSize < 0){
            printf("Didn't receive ack");
            exit(EXIT_FAILURE);
        }
        return -1;
    }

    strcpy(response,"response:node\nfile:yes");
    sendMsgSize = write(clientSock,response,strlen(response));
    if(sendMsgSize < 0)
    printf("write () error");
    
    int num = checkack(clientSock); 
    if(num < 0){
        printf("No ack recieved");
        exit(EXIT_FAILURE);
    }

     num = fstat(fd,&file_stat);
    if( num <0){
        printf("fstsat() error");
        exit(EXIT_FAILURE);
    }
    
    
    sprintf(buffer,"response:node\nfile:%s\nfilesize:%d",filename,(int)file_stat.st_size);
    sendMsgSize = write(clientSock,buffer,strlen(buffer));
    if(sendMsgSize < 0){
        printf("write() error while sending filesize response");
        exit(EXIT_FAILURE);
    }

     num = checkack(clientSock);
    if(num  < 0){
        printf("Didn't receive ack");
        exit(EXIT_FAILURE);
    }
    
    off_t offset = 0,remain_data = file_stat.st_size;
    float z = remain_data/1024.0;
    printf("File info - Filename : %s, Size : %.3f KB\n",filename,z);

    while((sendMsgSize = sendfile(clientSock,fd,&offset,BUFSIZ)) > 0 && remain_data){
        remain_data -= sendMsgSize;
        
        if(checkack(clientSock) < 0){
            printf("Didn't receive ack");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int ObtainFile(int clientSock,char * filename){
    int num, turn,recvMsgSize,sendMsgSize;   
    char buffer[1024];
    num=0;
    recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1);
   
    if( recvMsgSize < 0){
        printf("read() error nodes list size");
        exit(EXIT_FAILURE);
    }

    SendAck(clientSock,"client");
    FILE* fp = fopen(filename,"w");
    if(!fp)
    return -1;
    
    struct Container* packet = packet_parser(buffer);
    int buffer_size = atoi(getfieldvalue(packet,"filesize"));
    int remain_data = buffer_size;

    memset(buffer,'\0',sizeof(buffer));
    for (int i=1; ((recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1)) > 0 && (remain_data>0)) >0;i++)
   {
        fprintf(fp, "%s", buffer);
        remain_data -= recvMsgSize;
        
        memset(buffer,'\0',sizeof(buffer));
        SendAck(clientSock,"client");
        i--;
    }
    fclose(fp);
    return 0;
}

int KeepFile1(int clientSock,char* filename){
    struct stat file_stat; 
    int num ,turn,fd,recvMsgSize, sendMsgSize;
    num=0;
    char buffer[1024];
    fd = open(filename,O_RDONLY);
    if(fd < 0)
    return -1;
   
    turn = fstat(fd,&file_stat);
    if(turn<0){
        printf("fstsat() error");
        exit(EXIT_FAILURE);
    }
    
    if(num)
    printf("Delay in file");

    sprintf(buffer,"response:server\nfile:%s\nfilesize:%d",filename,(int)file_stat.st_size);
    sendMsgSize = write(clientSock,buffer,strlen(buffer));
    
    if(sendMsgSize < 0){
        printf("write() error while sending filesize");
        exit(EXIT_FAILURE);
    }
     memset(buffer,'\0',sizeof(buffer));
    recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1);
    if(recvMsgSize < 0){
        printf("read() error");
        exit(EXIT_FAILURE);
    }

    struct Container* packet = packet_parser(buffer);
    
    num =  atoi(packet[0].value);
    
    while(num>0 && strcmp(packet[--num].name,"ack")!=0);

    if(num<=0){
        printf("Didn't receive ack");
        exit(EXIT_FAILURE);
    }
    
    off_t offset = 0,remain_data = file_stat.st_size;

    while((sendMsgSize = sendfile(clientSock,fd,&offset,BUFSIZ)) > 0 && remain_data){
        remain_data = remain_data-sendMsgSize;
        
        memset(buffer,'\0',sizeof(buffer));
        recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1);
        if(recvMsgSize < 0){
            printf("read() error");
            exit(EXIT_FAILURE);
        }
        
        num = strcmp(getfieldvalue(packet_parser(buffer),"ack"),"1");
        if(num != 0){
            printf("No ack recieved");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int ObtainFile_(int clientSock,char * filename)
{
    char buffer[1024];
    int num, turn,recvMsgSize,sendMsgSize;

    recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1);
    if(recvMsgSize < 0)
        return -1;

    SendAck(clientSock,"client");
    FILE* fp = fopen(filename,"w");
    if(!fp)
    return -1;
    
    struct Container* packet = packet_parser(buffer);
    int buffer_size = atoi(getfieldvalue(packet,"filesize"));
    int remain_data = buffer_size;

    memset(buffer,'\0',sizeof(buffer));
    for(int i=1;((recvMsgSize = read(clientSock,buffer,sizeof(buffer) - 1)) > 0 && remain_data>0)>0;i++)
    {
        fprintf(fp, "%s", buffer);
        remain_data -= recvMsgSize;
        i--;
        memset(buffer,'\0',sizeof(buffer));
        SendAck(clientSock,"client");
    }
    fclose(fp);
    return 0;
}
