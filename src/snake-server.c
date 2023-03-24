#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include "helper.h" //functions that are used in imp.
int playerWinnerNumber=0; //indicate to all 
int currentNumberPlayer;
pthread_mutex_t gameStateLock = PTHREAD_MUTEX_INITIALIZER; //use to block thread until mutex unlocks
gamestate_t* gameState; //game state 
void* updateGameState(void* arg); //thread routing arg-> player socket df
int serverSocketFd; //server socket 
//clean exit
void ctrlCExitHandler(){
    close(serverSocketFd);
    printf("The server snaked out.\n");
    exit(0);
}
//write n bytes to descriptor
ssize_t writen(int fd, const void *vptr, size_t n);
//read n bytes to descriptor
ssize_t readn(int fd, void *vptr, size_t n);
int main(int argc, char *argv[])
{   
    signal(SIGINT,ctrlCExitHandler);//exit handler 
    srand(time(NULL));//use computer internal clock to control the choice of the seed -> prevents repetition between runs
    struct sockaddr_in serverAddress;//server socket address
    socklen_t clilen;
    gameState = (gamestate_t*) malloc (sizeof(gamestate_t));//allocate memory to game state
    memset(gameState,0,sizeof(gamestate_t));//fill game state to 0 -> 0 snakes and 0 fruits
    for(int i=0;i<6;i++)
        addFruit(&gameStateLock,gameState);//add 6 fruit to game

    /* Create server socket(AF_INET, SOCK_STREAM) */
    if ((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR : opening socket"); //error creation of socket failed 
        exit(1); //exit with a fail     
    }
    /* initialise sever socket - address + PORT*/
    memset(&serverAddress,0,sizeof(serverAddress)); //clear out socket structure 
    serverAddress.sin_family=AF_INET;//family of ip addresses used 
    serverAddress.sin_addr.s_addr=htonl(INADDR_ANY); //accept connections from any address
    serverAddress.sin_port=htons(PORT); //PORT defined to be 6969
    /* bind sever address to file descriptor */
    if (bind(serverSocketFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
         perror("ERROR : on binding");//binding failed
		 close(serverSocketFd);//close file descriptor so it no longer refers to no file 
         exit(-1);//exit with a fail value 
    }
    /*mark server socket as a socket that will be used to handle connection request*/
    if (listen(serverSocketFd, 5) < 0) {
        perror("ERROR on marking server socket as listening\n");//listening failed
        close(serverSocketFd);//close file descriptor so it no longer refers to no file 
        exit(1);//exit with a fail value 
    }
    else
    printf("Starting Server \n"); //indicate that server has been initiated
    clilen = sizeof(serverAddress);
    pthread_t thread;//create detached thread;
    int playerIndex = 0;
    while(true){
        int playerSocketFd;
        struct sockaddr_in playerAddress;
        if((playerSocketFd=accept(serverSocketFd,(struct sockaddr *)&playerAddress,&clilen))<0)
            error ("ERROR : connection with some player"); /*notify that a connection with some player failed*/
        /* if no error occurs  we are read to read/write from client and ready to accept connection with another player*/
            //check for a winner 
        if((gameState->playerWinner)!=0){
            gameState->playerWinner = 0; //reset win state (i.e. no winners in game)
            gameState->numberOfPlayers = 0;
            printf("Game has been refreshed\n");
        }
        gameState->numberOfPlayers++; //a player joined the game
        printf("Player %d joined the server\n",gameState->numberOfPlayers);//indicate that a player joined the server
        if(pthread_create(&thread,NULL,updateGameState,&playerSocketFd)<0)
            error("ERROR : creation of a new thread failed\n"); //handle error 
        if(pthread_detach(thread)<0) 
            error("Error : wait for thread termination failed\n"); //handle error
        playerIndex++;
    }
    close(serverSocketFd);
    return 0; 
}
//each player updates the current game state -> a thread for every player
void* updateGameState(void* arg){
    srand(time(NULL));
    int playerSocketFd = *(int *)arg;//get a hold of player updating game state
    int currentPlayerNumber = gameState->numberOfPlayers;//get player number
    int snakeIndex = currentPlayerNumber-1;
    char dirBuffer = DIR_RIGHT;
    bool endPlayer = false;
    int ret; //error handling
    snake_t* snake = makeSnake(&gameStateLock,gameState,snakeIndex); //implementation in helper.c
    gamestate_t gameStateNetworkByteOrdering; //to send over network
    while(!endPlayer){  
        /* check if player is the winner */
        if(gameState->snakes[snakeIndex].length>=WINNING_LENGTH){
            if((ret=pthread_mutex_lock(&gameStateLock))<0)
                perror("ERROR : mutex lock failed");
                gameState->playerWinner=WINNER_CODE;
            if((ret=pthread_mutex_unlock(&gameStateLock))<0)
                perror("ERROR : mutex unlock failed");
            playerWinnerNumber=currentPlayerNumber;
            endPlayer=true;
        }
        else if(gameState->playerWinner!=0){
            if((ret=pthread_mutex_lock(&gameStateLock))<0)
                perror("ERROR : mutex lock failed");
                gameState->playerWinner=playerWinnerNumber;
            if((ret=pthread_mutex_unlock(&gameStateLock))<0)
                perror("ERROR : mutex unlock failed");
                endPlayer=true; 
        }
        /* ensure correct endianess  host to network byte order  SEND */
        memset(&gameStateNetworkByteOrdering,0,sizeof(gamestate_t)); //clear bits to 0
        gameStateNetworkByteOrdering.numberOfPlayers=htonl(gameState->numberOfPlayers); 
        gameStateNetworkByteOrdering.playerWinner=htonl(gameState->playerWinner);
        for(int i=0;i<MAX_FRUITS;i++){
            gameStateNetworkByteOrdering.fruits[i].x=htonl(gameState->fruits[i].x);
            gameStateNetworkByteOrdering.fruits[i].y=htonl(gameState->fruits[i].y);
        }
        for(int i=0;i<MAX_PLAYERS;i++){
            gameStateNetworkByteOrdering.snakes[i].isDead=htons(gameState->snakes[i].isDead);
            gameStateNetworkByteOrdering.snakes[i].length=htonl(gameState->snakes[i].length);
            gameStateNetworkByteOrdering.snakes[i].head.x=htonl(gameState->snakes[i].head.x);
            gameStateNetworkByteOrdering.snakes[i].head.y=htonl(gameState->snakes[i].head.y);
            for(int j=0;j<(MAX_SNAKE_LEN)-1;j++){
                gameStateNetworkByteOrdering.snakes[i].tail[j].x=htonl(gameState->snakes[i].tail[j].x);
                gameStateNetworkByteOrdering.snakes[i].tail[j].y=htonl(gameState->snakes[i].tail[j].y);
            }
        }
        if(writen(playerSocketFd,(char*)&gameStateNetworkByteOrdering,sizeof(gamestate_t))<=0)
            break;
        if(readn(playerSocketFd,&dirBuffer,sizeof(char))<=0)
            break;
        moveSnake(&gameStateLock,gameState,snake,snakeIndex,dirBuffer);//update game state with next move                                                                 
        /* check if snake died with move (hit boundary or another snake) */
        if(snake->isDead)       
            endPlayer=true;//snake died
        }
        //close connection
        close(playerSocketFd);
        if((ret=pthread_mutex_lock(&gameStateLock))<0)
            perror("ERROR : mutex lock failed");
            memset(&(gameState->snakes[snakeIndex]),0,sizeof(snake_t)); //clear snake from game map
        if((ret=pthread_mutex_unlock(&gameStateLock))<0)
            perror("ERROR : mutex unlock failed");
        free(snake);
}
ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
    if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
        if (nwritten < 0 && errno == EINTR)
            nwritten = 0;   /* and call write() again */
        else
            return (-1);    /* error */
    }
    nleft -= nwritten;
    ptr += nwritten;
    }
    return (n);
}
ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    size_t nread;
    char   *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) nread = 0;      /* and call read() again */
            else return (-1);
        } else if (nread == 0)  break;              /* EOF */
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
