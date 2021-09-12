#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

struct Holder
{
    char* name;
    char* value;
};

struct Holder * packet_parser(char *space){
    int fields_num = 1;
    struct Holder* fieldofPackets = (struct Holder*)malloc(sizeof(struct Holder)*32);
    char* temp = strtok(space,"\n");
    while(temp){
        fieldofPackets[fields_num++].name = temp;
        temp = strtok(NULL,"\n");
    }

    char* fields_num_string = (char*)malloc(sizeof(char)*32);
    sprintf(fields_num_string,"%d",fields_num);
    fieldofPackets[0].value = fields_num_string;
    fieldofPackets[0].name = NULL;
    

    int i=1;
    while(i<fields_num){
        temp = strtok(fieldofPackets[i].name,":");
        fieldofPackets[i].name = temp;
        temp = strtok(NULL,":");
        fieldofPackets[i].value = temp;
        i++;
    }

    return fieldofPackets;
}

char * getfileval(struct Holder* packet,char *field){
    int n = atoi(packet[0].value);
    while(n>0 && strcmp(packet[--n].name,field)!=0);
    if(n<=0){
        return NULL;
    }
    return packet[n].value;
}
int ackcheck(int SocketClient){
    int receivedMessageSize;
    char space[1024];
    memset(space,'\0',sizeof(space));
    receivedMessageSize = read(SocketClient,space,sizeof(space) - 1);
    if(receivedMessageSize < 0){
        return -1;
    }
    if(strcmp(getfileval(packet_parser(space),"ack"),"1") != 0){
        return -1;
    }
    return 0;
}
int FileKeep(int SocketClient,const char * filename){
    char response[128];
    int recievedMessageSize, sendMessageSize;
    struct stat file_stat;
    int fd;
    
    if((fd = open(filename,O_RDONLY)) < 0){
        strcpy(response,"response:node\nfile:#####");
        if((sendMessageSize = write(SocketClient,response,strlen(response))) < 0){
            printf("Unable to send the response\n");
            exit(EXIT_FAILURE);
        }
    
        if(ackcheck(SocketClient) < 0){
            printf("Acknowlegement not recieved");
            exit(EXIT_FAILURE);
        }
        return -1;
    }

    strcpy(response,"response:node\nfile:yes");
    if((sendMessageSize = write(SocketClient,response,strlen(response))) < 0){
        printf("Unable to write");
    }

    if(ackcheck(SocketClient) < 0){
        printf("Acknowlegement not recieved");
        exit(EXIT_FAILURE);
    }


    if(fstat(fd,&file_stat)<0){
        printf("fstsat() function error");
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    sprintf(buffer,"response:node\nfile:%s\nfilesize:%d",filename,(int)file_stat.st_size);

    if((sendMessageSize = write(SocketClient,buffer,strlen(buffer))) < 0){
        printf("Unable to write in Message Size");
        exit(EXIT_FAILURE);
    }

    if(ackcheck(SocketClient) < 0){
        printf("No ack recieved");
        exit(EXIT_FAILURE);
    }
    
    off_t offset = 0,remain_data = file_stat.st_size;
    float z = remain_data/1024.0;
    printf("File info - Filename : %s, Size : %.3f KB\n",filename,z);

    while((sendMessageSize = sendfile(SocketClient,fd,&offset,BUFSIZ)) > 0 && remain_data){
        remain_data -= sendMessageSize;
        if(ackcheck(SocketClient) < 0){
            printf("No ack recieved");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
int AcknowlegementSend(int SocketClient,char *pre){
    char buffer[1024];
    int MessageSize;
    if(strcmp(pre,"client") == 0)    strcpy(buffer,"response:client\nack:1");
    else    strcpy(buffer,"response:node\nack:1");
    if((MessageSize = write(SocketClient,buffer,strlen(buffer))) < 0){
        if(strcmp(pre,"node") == 0)
            printf("request write() ack error");
        else
            printf("write() ack send error");
        exit(EXIT_FAILURE);
    }
}
void utilityServer(int socketOfClient,char *ipOfClient,int portOfClient){
    char bufferSpace[1024];
    int sendMessageSize,recievedMessageSize;
    memset(bufferSpace,'\0',sizeof(bufferSpace));
    recievedMessageSize = read(socketOfClient,bufferSpace,sizeof(bufferSpace) - 1);
    if(!(recievedMessageSize >= 0)){
        printf("Unable to read");
        exit(EXIT_FAILURE);
    }
    if(strcmp(bufferSpace,"request:client") != 0){
        printf("Unexpected request received");
        exit(EXIT_FAILURE);
    }
    AcknowlegementSend(socketOfClient,"node");
    memset(bufferSpace,'\0',sizeof(bufferSpace));
    recievedMessageSize = read(socketOfClient,bufferSpace,sizeof(bufferSpace) - 1);
    if( recievedMessageSize < 0){
        printf("read() error");
        exit(EXIT_FAILURE);
    }
    AcknowlegementSend(socketOfClient,"node");
    char* fileValue;
    fileValue = getfileval(packet_parser(bufferSpace),"filename");
    printf("'%s' file request has been recieved from %d : %d\n", fileValue,ipOfClient,portOfClient);
    if(!(FileKeep(socketOfClient,fileValue) == 0)){
        fprintf(stdout,"Unable to send the file\n");
    }
    else{
        fprintf(stdout,"File '%s' sent\n", fileValue);
    }
}

void noderServerStart(int pNumber){

    int socketOfServer;
    int PortOfServer = pNumber;
    struct sockaddr_in AddressOfServer;
    int i;
    socketOfServer=socket(AF_INET,SOCK_STREAM, 0);
    if( socketOfServer < 0){
        printf("Unable to buld the socket");
        exit(EXIT_FAILURE);
    }
    memset((void *)&AddressOfServer,'\0',sizeof(AddressOfServer));
    AddressOfServer.sin_port = htons(PortOfServer);
    AddressOfServer.sin_addr.s_addr = htonl(INADDR_ANY);
    AddressOfServer.sin_family = AF_INET;

    if(!(bind(socketOfServer,(struct sockaddr*) &AddressOfServer, sizeof(AddressOfServer)) >= 0)){
        printf("Unable to find");
        exit(EXIT_FAILURE);
    }

    if(!(listen(socketOfServer,5) >= 0)){
        printf("Unable to Listen");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Node server listening on port %d\n",PortOfServer);
    }

    for(i=1;i>0;){
        struct sockaddr_in clientAddress;
        int socketOfClient,clientLength;
        int portOfClient;

        clientLength = sizeof(clientAddress);

        
        char ipOfClient[INET_ADDRSTRLEN];
        if(!((socketOfClient=accept(socketOfServer,(struct sockaddr *)&clientAddress,&clientLength)) >= 0)){
            printf("Unable to accept");
            exit(EXIT_FAILURE);
        }
        else{
            memset(ipOfClient,'\0',sizeof(ipOfClient));
            if(!(inet_ntop(AF_INET, &(clientAddress.sin_addr),ipOfClient,sizeof(ipOfClient)) != 0)){
                printf("Error in inet_ntop()");
                exit(EXIT_FAILURE);
            }
            portOfClient = ntohs(clientAddress.sin_port);
            printf("Client %s:%d accepted\n", ipOfClient,portOfClient);
        }

        int portID = fork();

        if(portID < 0){
            printf("Unable to fork");
            exit(EXIT_FAILURE);
        }

        if(portID == 0){
            close(socketOfServer);
            utilityServer(socketOfClient,ipOfClient,portOfClient);
            exit(EXIT_SUCCESS);
        }
        else{
            close(socketOfClient);
        }
    }
}

int main(int argc,char **argv)
{
    
    int SocketNode;
    if(argc!=3){
        printf("The number of arguments are not 3");
        exit(EXIT_FAILURE);
    }

    char* IpOfServer = argv[1];
    int PortOfServer = atoi(argv[2]);
    struct sockaddr_in AddressOfServer;
    SocketNode = socket(AF_INET, SOCK_STREAM, 0);
    if(SocketNode < 0){
        printf("Unable to build socket");
        exit(EXIT_FAILURE);
    } 
    memset(&AddressOfServer, '\0', sizeof(AddressOfServer)); 
    
    AddressOfServer.sin_family = AF_INET;
    AddressOfServer.sin_port = htons(PortOfServer);
    AddressOfServer.sin_addr.s_addr = inet_addr(IpOfServer);
    
     int connectionStatus;
     connectionStatus=connect(SocketNode, (struct sockaddr *)&AddressOfServer, sizeof(AddressOfServer));
    if( connectionStatus < 0){
       printf("connect function has been failed");
       exit(EXIT_FAILURE);
    }
    else{
        printf("Connected to server with IP %s and port %d\n",IpOfServer,PortOfServer);
    }

    char* request = "request:node";
    int recievedMessageSize,sendMessageSize;
    
    
    sendMessageSize=write(SocketNode,request,strlen(request));
    if(sendMessageSize < 0){
        printf("Unable to write");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Registering with server having IP %s and port no %d\n",IpOfServer,PortOfServer);
    }
    char bufferSpace[1024];
    memset(bufferSpace,'\0',sizeof(bufferSpace));
    
    recievedMessageSize = read(SocketNode,bufferSpace,sizeof(bufferSpace) - 1);
    if(recievedMessageSize< 0){
        printf("Unable to read");
        exit(EXIT_FAILURE);
    }
    char* responseValue = strtok(bufferSpace,"\n");
    char* statusValue = strtok(NULL,"\n");
    char* portValue = strtok(NULL,"\n");
    printf(" kkk\n");
    if(strcmp(responseValue,"response:server") != 0 || strcmp(statusValue,"status:connected")!=0){
        printf("Received an unexpected response from server");
        exit(EXIT_FAILURE);
    }
    else{
        portValue = strtok(portValue,":");
        portValue = strtok(NULL,":");
        if(shutdown(SocketNode,0)< 0){
            printf("Unable to Shutdown node");
            exit(EXIT_FAILURE);
        }
        else{
            printf("Registered with server %s:%d\n",IpOfServer,PortOfServer);
            printf("Gracefully closing connection with server %s:%d\n",IpOfServer,PortOfServer );
        }
        int ServernodePort = atoi(portValue);
        noderServerStart(ServernodePort);
    }
    return 0;
}

