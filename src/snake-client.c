#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include "helper.h"
#include  <errno.h>
ssize_t writen(int fd, const void *vptr, size_t n);
ssize_t readn(int fd, void *vptr, size_t n);
WINDOW* gameWindow;
void* updateServer(void* arg);
void* readServer(void* arg);
char startWindow(WINDOW* win);
char currentDirection = DIR_RIGHT;
bool gameOver = false;
int winner;
int sockfd;
void ctrlCExitHandler(){
    
    close(sockfd);
    printf("The server snaked out.\n");
    exit(0);
}
int main(int argc, char **argv){
    winner=0;
    int ret;
    /* Make sure server name is included as a command line argument, port number 
    already predefined so no need to specify.*/
    if (argc < 2) {
        fprintf(stderr, "Error : Usage %s <some IP address that the server resolves to>\n", argv[0]);
        exit(0);
    }
    /* Declare variables */
    struct sockaddr_in serv_addr;
    struct hostent *server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("ERROR : opening socket (creating file descriptor)");
    /* Get server name */
    if ((server = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr,"ERROR : no host name resolves to %s \n",argv[1]);
		close(sockfd);
        exit(0);
    }
    /* Populate serv_addr structure  */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  // Set to AF_INET 
    memcpy(&serv_addr.sin_addr.s_addr,
            server -> h_addr, // Set server address
            server -> h_length);
    serv_addr.sin_port = htons(PORT); // Set port (convert to network byte ordering)
    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
         close(sockfd);
         error("ERROR : connecting");
    }
    //init ncurses
    //display game window
    initscr();//start ncurses
    timeout(100);//timeout
    cbreak();//disable line feed buffering
    curs_set(0);//cursor - no visibility mode
    start_color();//allow use of colours
    noecho();//do not echo characters on terminals
    if(stdscr->_maxx<COLUMNS&&stdscr->_maxy<ROWS){
        fprintf(stderr,"Terminal size not supported (too small)\n");
        exit(EXIT_FAILURE); 
    }
    if(init_pair(GAME_WINDOW_COLOUR,COLOR_BLACK,COLOR_WHITE)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of black foreground and white background failed");
        exit(1);
    }   
    if(init_pair(RED,COLOR_RED,COLOR_WHITE)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of red foreground and white background failed");
        exit(1);
    }   
    if(init_pair(GREEN,COLOR_GREEN,COLOR_WHITE)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of green foreground and white background failed");
        exit(1);
    }  
    if(init_pair(FRUIT_COLOUR,COLOR_WHITE,COLOR_RED)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of white foreground and red background failed");
        exit(1);
    }
    if(init_pair(SNAKE_COLOUR_1,COLOR_WHITE,COLOR_GREEN)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of white foreground and green background failed");
        exit(1);
    }
    if(init_pair(SNAKE_COLOUR_2,COLOR_WHITE,COLOR_MAGENTA)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of white foreground and magenta background failed");
        exit(1);
    }
    if(init_pair(SNAKE_COLOUR_3,COLOR_WHITE,COLOR_BLUE)==ERR){
        delwin(gameWindow);
        echo(); 
        curs_set(1);  
        endwin();
        fprintf(stderr,"ERROR : colour definition of white foreground and blue background failed");
        exit(1);
    }
    gameWindow = newwin(ROWS,COLUMNS,0,0);
    wbkgd(gameWindow,COLOR_PAIR(GAME_WINDOW_COLOUR));
    char c = startWindow(gameWindow);
    //if not q - continue
    if(toupper(c)!=toupper('q')){
        pthread_t readThread;
        pthread_t writeThread;
        if(pthread_create(&writeThread,NULL,updateServer,&sockfd)<0){
            close(sockfd);
            error("ERROR : creation of a new thread failed"); //handle error 
        }
        if(pthread_detach(writeThread)<0){
            close(sockfd);
            error("Error : detach thread failed"); //handle error
        }
        if(pthread_create(&readThread,NULL,readServer,&sockfd)<0){
            close(sockfd);
            error("ERROR : creation of a new thread failed"); //handle error 
        }
        if(pthread_detach(readThread)<0){
            close(sockfd);
            error("Error : detach thread failed"); //handle error
        }
        char currentKeyPressed;
        while(!gameOver){
            currentKeyPressed = getch();
            if(toupper(currentKeyPressed) == 'Q'){
                exit(0);

                break;
            }
            else{
                if(toupper(currentKeyPressed)=='W')
                    currentDirection=DIR_UP;
                else if(toupper(currentKeyPressed)=='S')
                    currentDirection=DIR_DOWN;
                else if(toupper(currentKeyPressed)=='A')
                    currentDirection=DIR_LEFT;
                else if(toupper(currentKeyPressed)=='D')
                    currentDirection=DIR_RIGHT;
            }
         }


}
    close(sockfd);
    return 0;
}
void* updateServer(void* arg){
    int ret=-1;
    /* send current direction to the server */
    int sockfd = *(int*) arg; //get socket file descriptor
    /* continuously write to server while game is not over */
    struct timespec ts;
    ts.tv_sec = 0.1;
    ts.tv_nsec = 100000000;
    while(true){
        nanosleep(&ts, NULL);
        // Send message to the server
        int n;
        if(writen(sockfd,&currentDirection,sizeof(char))<=0){
        	gameOver=true;
        	break;
        }
 
        // if ((n = write(sockfd,&currentDirection,sizeof(char)) < 0)) 
        //  {   
        //      close(sockfd);
        //      error("ERROR : writing to socket");
        //  }  
    }     
    pthread_exit(NULL);
}
char startWindow(WINDOW* win){
    if(win==NULL)
    puts("not null");
    wattron(win, COLOR_PAIR(GREEN));
    centerText(win,1,"               |         ");
    centerText(win,2,",---.,---.,---.|__/ ,---.");
    centerText(win,3,"`---.|   |,---||  \\ |---'");
    centerText(win,4,"`---'`   '`---^`   ``---'");
    wattroff(win, COLOR_PAIR(GREEN));;
    centerText(win,5,"You successfully connected to the server.");
    centerText(win,7,"Press S to start the game.");
    centerText(win,8,"Press Q to quit the game.");
    centerText(win,10,"Controls:");
    centerText(win,11,"w -> up, s -> down   ");
    centerText(win,12,"a -> left, d -> right");
    centerText(win,13,"Developed by Andre Vella.");
    centerText(win,14,"In fulfillment");
    centerText(win,15,"of unit CPS2008.");
    box(win,0,0);
    refresh();
    wrefresh(win);
    timeout(-1);
    char input = getch();
    wattron(win,COLOR_PAIR(RED));
    while((input!='q'&&input!='Q')&&(input!='s'&&input!='S')){
        timeout(1000);
        input = getch();
        centerText(win,18,"Error invalid input");
        mvwaddch(win,18,win->_maxx/2+11 ,input);
        mvwaddch(win,18,win->_maxx/2+12 ,'.');
        refresh();
        wrefresh(win);
    }
    wattroff(win,COLOR_PAIR(RED));
    return input;
}
void* readServer(void* arg){  
    int  sockfd = *(int*) arg;
    int  bytes_read;
    gamestate_t gameState;
    char* gameStateBuffer = (char*)malloc(sizeof(gamestate_t));
    int  i, j, n;
    memset(gameStateBuffer, 0, sizeof(gamestate_t));
    werase(gameWindow);   
    endwin();
    bool continueReading;
    int playerNumber=0;
    //readn(sockfd,&gameStateBuffer,sizeof(gamestate_t));
    // memcpy(&gameState, gameStateBuffer, sizeof(gamestate_t));
    //playerNumber=ntohl(gameState.numberOfPlayers);
    bool getPlayerNumberOfPlayersOnce=true;
    while(true){ 
        wclear(gameWindow);
        box(gameWindow,0, 0);
        int n=0;
        timeout(1);

        
        if(readn(sockfd,gameStateBuffer,sizeof(gameState))<=0){
            gameOver=true;
            if(ntohl(gameState.playerWinner)!=0)
                winner=ntohl(gameState.playerWinner);
            break;
        }
        else{
        memcpy(&gameState, gameStateBuffer, sizeof(gamestate_t));
        //dispay player number
        if(getPlayerNumberOfPlayersOnce){
                playerNumber=ntohl(gameState.numberOfPlayers);
                getPlayerNumberOfPlayersOnce=false;
            }
            mvwprintw(gameWindow,19,2,"You are player %d",playerNumber);

        
        //display fruits
        for(int i=0;i<MAX_FRUITS;i++)
            if(ntohl(gameState.fruits[i].x)!=0&&ntohl(gameState.fruits[i].y)!=0){
                 wattron(gameWindow,COLOR_PAIR(FRUIT_COLOUR));
                 mvwprintw(gameWindow,ntohl(gameState.fruits[i].y), ntohl(gameState.fruits[i].x), "f"); 
                 wattroff(gameWindow,COLOR_PAIR(FRUIT_COLOUR));
            }
        //display snakes 
        for(int i=0;i<ntohl(gameState.numberOfPlayers);i++){
            for(int j=0;j<ntohl(gameState.snakes[i].length);j++){
                if(j==0){
                    wattron(gameWindow,COLOR_PAIR(SNAKE_COLOUR_1+(i%3)));
                    mvwprintw(gameWindow,ntohl(gameState.snakes[i].head.y),ntohl(gameState.snakes[i].head.x),":)");
                    wattroff(gameWindow,COLOR_PAIR(SNAKE_COLOUR_1+(i%3)));
                }   
                else{
                    if(ntohl(gameState.snakes[i].tail[j-1].y)!=0&&ntohl(gameState.snakes[i].tail[j-1].x!=0)){
                        wattron(gameWindow,COLOR_PAIR(SNAKE_COLOUR_1+(i%3)));
                        mvwprintw(gameWindow,ntohl(gameState.snakes[i].tail[j-1].y),ntohl(gameState.snakes[i].tail[j-1].x),"  ");
                        wattroff(gameWindow,COLOR_PAIR(SNAKE_COLOUR_1+(i%3)));
                    }
                }   
            } 
        }
        }
        refresh();
        wrefresh(gameWindow);  
    }
    wclear(gameWindow);
    echo(); 
    curs_set(1);  
    endwin();
        if (winner==WINNER_CODE){
        printf("You Won - GG \n");
      
    } else{
        printf("RIP - you got owned !\n");
        if(winner>0){
            printf("Player %d ate the whole game \n",winner);
        }

    }

    pthread_exit(NULL);
}
/* taken from https://www.informit.com/articles/article.aspx?p=169505&seqNum=9 (Unix Socket Programming API book)*/
ssize_t /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
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
ssize_t                         /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
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
