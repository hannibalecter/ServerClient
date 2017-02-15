#include <stdio.h>
#include <string.h>    // for strlen
#include <sys/socket.h>
#include <arpa/inet.h> // for inet_addr
#include <stdlib.h>
#include <unistd.h>    //write
#include <pthread.h>
#define NRM  "\x1B[0m"
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define localhost "127.0.0.1"
#define port_no 8888

// All of Function
void xor(char *key, char *string, int n);
unsigned char swap_nibbles(unsigned char msg);
void hexConvertMessage(char* msg, int messageLength);
unsigned char swapMessage;
char key[200];
// Receive thread is created for server message
void *Receive(void *socket_desc)
{
    // Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[2000];

    //Receive a msg from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
      int flagError = 0;
        client_message[read_size] = '\0';
        char tempKey[2000];
        // Miss data control
        if(strcmp(client_message, "Missing data entered!") == 0){
          flagError = 1;
          printf("%sMissing data entered!%s\n",RED,NRM);
          printf(">> ");
        }
        else if(strcmp(key ,"") == 0 ){
            strcpy(key, client_message);
        }
        else
        { // Decode server message
            xor(key, client_message, read_size);
            int i;
            for(i=0; i < read_size; i++){
            client_message[i] = swap_nibbles(client_message[i]);
            }
        }

        if(!flagError && strlen(client_message) < 190)
          printf("Server's replay: %s\n", client_message);

        // Clear the msg buff.
        memset(client_message, 0, 2000);
    }


}

int main(int argc, char *argv[])
{ // Socket variables
    int socket_desc;
    struct sockaddr_in server;
    char msg[2000], server_reply[2000];
    pthread_t thread_id;
    // created socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        puts("Could not create socket");
        return 1;
    }
    server.sin_addr.s_addr = inet_addr(localhost);
    server.sin_family = AF_INET;
    server.sin_port = htons(port_no);
    // For connection server
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("Connection error");
        return 1;
    }

    puts("Client connected");

      if( pthread_create(&thread_id, NULL, Receive, (void*) &socket_desc) < 0)
        {
            perror("Could not create thread");
            return 1;
        }


    while(1)
    {
        char *buffer;
        size_t bufsize = 2000;
        size_t characters;
        // For sended message at server
        printf("\n>> ");
        fgets(msg, 2000, stdin);
        msg[strcspn(msg, "\n")]=0;

        int sizeMessage = strlen(msg);
        //printf("Sended message : %s \n", msg);
        int i;


        if(strcmp(key ,"") != 0){
            // Encode Message
            for(i = 0; i < sizeMessage; i++){
                msg[i] = swap_nibbles((unsigned char)msg[i]);
            }
            xor(key, msg, sizeMessage); // For security key
            //hexConvertMessage(key,sizeMessage);
            if(send(socket_desc , msg , sizeMessage , 0) < 0)
            {
                puts("Send failed");
                return 1;
            }
            // Receive the reply from localhost
        }
        else{

            if(send(socket_desc , msg , sizeMessage , 0) < 0)
            {
                puts("Send failed");
                return 1;
            }
        }// Clear the msg buff.
        memset(msg, 0, 2000);
        memset(server_reply, 0, 2000);
    }

    close(socket_desc);
    return 0;
}
// XOR for two string for key
void xor(char *key, char *string, int n){
    // For xor
    int i;
    int keyLength = strlen(key);
    for( i = 0 ; i < n ; i++ )
    {
        string[i]=string[i]^key[i%keyLength];
    }

}
// Swap bits function
unsigned char swap_nibbles(unsigned char msg)
{
     unsigned char temp1, temp2;

     temp1 = msg & 0x0F;
     temp2 = msg & 0xF0;
     temp1 = temp1 << 4;
     temp2 = temp2 >> 4;

     return(temp2|temp1); //adding the bits
}
 // Normal char message convert hex
void hexConvertMessage(char* msg, int messageLength){

    int i;
    for (i = 0; i < messageLength; i++)
    {
        printf("%02X", msg[i]);
    }

}
