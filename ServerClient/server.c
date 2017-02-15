
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#define NRM  "\x1B[0m"
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"

// Client struct is created
typedef struct _Client{

    char clientName[100];
    int socketID;
    char key[200];

} Client;
// Alias struct is created
typedef struct _ClientAlias{
    char aliasName[100];
    Client clientsOfAlias[100];

} ClientAlias;

Client clientArr[100];
ClientAlias clientAliasArr[300];
int counter = 0;
int counterAlias = 0;

// All of Function

int searchAlias(char* aliasName, ClientAlias* clientAlias);
int searchClient(char* name, Client* client);
void *connection_handler(void *);
void displayClients(char* client);
void randomGenerator(int keyLength, char* secretKey);
unsigned char swap_nibbles(unsigned char msg);
void xor(char *key, char *string, int n);
void hexConvertMessage(char* msg, int messageLength);
int searchSocketID(int id, Client* client);

int main(int argc , char *argv[])
{
    // Variables for socket connection
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;

    // Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("%sCould not create socket%s \n",RED,NRM);
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    // For Bind accept
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        // To print error message

        perror("Bind failed. Error");

        return 1;
    }
    printf("%sServer Started \n",GRN);
    // For Listen
    listen(socket_desc , 3);

    // Accept and incoming connection
    c = sizeof(struct sockaddr_in);


    // Accept and incoming connection
    printf("Waiting for clients...%s\n",NRM);
    c = sizeof(struct sockaddr_in);
    pthread_t thread_id;

    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("%sClient Connection accepted%s \n",GRN,NRM);
        if(pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("Could not create thread");
            return 1;
        }
    }

    if (client_sock < 0)
    {
        perror("Accept failed");
        return 1;
    }

    return 0;
}
char tempName[100];
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char msg[2000], client_message[2000];
    int flag = 0;
    char key[200];
    //Receive a msg from client
    while((read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
        client_message[read_size] = '\0';
        // Decode for client message
        if(flag){
            xor(key, client_message, read_size);
            int i;
            for(i=0; i < read_size; i++){

                client_message[i] = swap_nibbles(client_message[i]);
            }
        }
        //printf("%s\n", client_message);
        char* tokens = strtok(client_message, " ");

        // Login control
        if(strcmp(tokens, "login") == 0){
          // For multiple flights in the same terminal
          if(!flag){
            tokens = strtok(NULL, " ");
            if(tokens == NULL){
                strcpy(key,"Missing data entered!");
                write(sock, key, strlen(key));
            }
            else{
              char tempToken[2000];
              strcpy(tempToken, tokens);
              int sizeMessage = strlen(tempToken);
                flag = 1;
                //printf("asdasd\n");
              strcpy(clientArr[counter].clientName, tempToken);
              clientArr[counter].socketID = sock;
              randomGenerator(200, key);
              strcpy(clientArr[counter].key, key);
              int keySize = strlen(key);
              write(sock, key, keySize);
              // for number of user

              puts(GRN);
              printf("Client logged in as %s \n", tempToken);
              puts(NRM);
              printf("Secret key generated for %s as: %s\n\n",clientArr[counter].clientName,clientArr[counter].key);
              counter++;
            }
          }
        } // Getusers Control
        else if(flag && strcmp(tokens, "getusers") == 0){
            //printf("asdasdad\n");
            displayClients(msg);
            printf("User %s's show all of users(getusers)\n", clientArr[counter-1].clientName);
            // Encode
            int sizeMessage = strlen(msg);
            int i;
            for(i = 0; i < sizeMessage; i++){
                msg[i] = swap_nibbles((unsigned char)msg[i]);
            }
            xor(key, msg, sizeMessage); // For security key

            write(sock , msg , sizeMessage);
        } // Occur group Control
        else if(flag && strcmp(tokens, "alias") == 0){
            tokens = strtok(NULL, " ");
            strcpy(clientAliasArr[counterAlias].aliasName, tokens);
            printf("%sGroup '%s' is created with ",GRN,tokens );
            tokens = strtok(NULL, " ");
            char* tokenAlias = strtok(tokens, ",");
            int j = 0;

            while(tokenAlias != NULL){
                Client temp;
                searchClient(tokenAlias, &temp);
                clientAliasArr[counterAlias].clientsOfAlias[j] = temp;
                printf("%s,", temp.clientName);
                j++;
                tokenAlias = strtok(NULL, ",");
            }
            printf("%s\n",NRM);
            counterAlias++;
        } // Single or group client message control
        else if(flag){

            Client client;
            ClientAlias alias;
            char name[100];
            strcpy(name, tokens);
            tokens = strtok(NULL, "\0"); // For msg combine

            if(searchClient(name, &client)){

                int sizeMessage = strlen(tokens);
                int i; // TODO: users message
                Client tempC;
                searchSocketID(sock, &tempC);
                printf("User %s send a message: ", tempC.clientName);
                hexConvertMessage(tokens, sizeMessage);
                printf("\n");
                printf("User %s’s message is decoded as: %s\n", tempC.clientName, tokens);
                for(i = 0; i < sizeMessage; i++){ // swap for
                    tokens[i] = swap_nibbles((unsigned char)tokens[i]);
                }
                xor(client.key, tokens, sizeMessage); // For security key

                write(client.socketID , tokens , sizeMessage);
            } // Alias message control
            else if(searchAlias(name, &alias)){
                int i;
                char tempToken[2000];
                Client tempC;
                searchSocketID(sock, &tempC);
                printf("User %s send a message at '%s' group.\n",tempC.clientName,name);
                for(i = 0; i < 100; i++){
                    if(strcmp(alias.clientsOfAlias[i].clientName, "") != 0){
                        strcpy(tempToken, tokens);
                        int sizeMessage = strlen(tempToken);
                        int j;
                        for(j = 0; j < sizeMessage; j++){
                            tempToken[j] = swap_nibbles((unsigned char)tempToken[j]);
                        }
                        xor(alias.clientsOfAlias[i].key, tempToken, sizeMessage); // For security key


                        write(alias.clientsOfAlias[i].socketID , tempToken, sizeMessage);
                    }


                }
            }
        }// Clear the msg buff.
        memset(msg, 0, 2000);
        memset(client_message, 0, 2000);
    }

    if(read_size == 0)
    {
        printf("%sClient disconnected%s\n",RED,NRM);
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("Recv failed");
    }

    return 0;
}
// All of client print screen
void displayClients(char* client){
    int i;
    for(i = 0; i < counter; i++){

        strcat(client, clientArr[i].clientName);
        strcat(client, ",");
    }
}
// Search function for client
int searchClient(char* name, Client* client){
    int i;
    for(i = 0; i < counter; i++){

        if(strcmp(clientArr[i].clientName, name) == 0){
            strcpy(client->clientName, clientArr[i].clientName);
            client->socketID = clientArr[i].socketID;
            strcpy(client->key, clientArr[i].key);
            return 1;
        }
    }
    return 0;
}
int searchSocketID(int id, Client* client){
    int i;
    for(i = 0; i < counter; i++){

        if(clientArr[i].socketID == id){
            strcpy(client->clientName, clientArr[i].clientName);
            client->socketID = clientArr[i].socketID;
            strcpy(client->key, clientArr[i].key);
            return 1;
        }
    }
    return 0;
}

// Search function for group(alias)
int searchAlias(char* aliasName, ClientAlias* clientAlias){
    int i;
    for(i = 0; i < counterAlias; i++){
        if(strcmp(clientAliasArr[i].aliasName, aliasName) == 0){
            strcpy(clientAlias->aliasName, clientAliasArr[i].aliasName);
            int j;
            for(j = 0; j < 100; j++){
                clientAlias->clientsOfAlias[j] = clientAliasArr[i].clientsOfAlias[j];

            }
            return 1;
        }
    }
    return 0;
} // Create random key for messages
void randomGenerator(int keyLength, char* secretKey){
    char symbols[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','*','~','<','>','/','|','+','#','!','%','^','?','-','_','[',']','(',')','{','}'};

    if (!secretKey) {
        return 0;
    }
    unsigned int select = 0;
    int i;
    for (i = 0; i < keyLength; i++) {
        select = rand() % sizeof(symbols);
        secretKey[i] = symbols[select];
    }
    secretKey[keyLength] = '\0';
} // XOR for two string for key
void xor(char *key, char *string, int n){
    // For xor
    int i;
    int keyLength = strlen(key);
    for( i = 0 ; i < n ; i++ )
    {
        string[i]=string[i]^key[i%keyLength];
    }

} // Swap bits function
unsigned char swap_nibbles(unsigned char msg)
{
     unsigned char temp1, temp2;

     temp1 = msg & 0x0F;
     temp2 = msg & 0xF0;
     temp1 = temp1 << 4;
     temp2 = temp2 >> 4;

     return(temp2|temp1); //adding the bits
} // Normal char message convert hex
void hexConvertMessage(char* msg, int messageLength){

    int i;
    for (i = 0; i < messageLength; i++)
    {
        printf("%02X", msg[i]);
    }

}
