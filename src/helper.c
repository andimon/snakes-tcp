#include "helper.h"
#include <ncurses.h>
#include "string.h"
/* print text centered on some window */
void centerText(WINDOW* win,int row,const char* str){
    mvwaddstr(win,row,(win->_maxx)/2-strlen(str)/2,str);
}
void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
void addFruit(pthread_mutex_t* gameStateLock,gamestate_t* gameState){
        /* check if max fruits appear in the gameboard */
        unsigned short int fruitIndex=0;
        bool maxFruitsOnGameBoard;
        for(;fruitIndex<MAX_FRUITS;fruitIndex++)
            if((gameState->fruits[fruitIndex].x==0)&&(gameState->fruits[fruitIndex].y==0))
                break;
        if(fruitIndex==MAX_FRUITS)
            fprintf(stderr,"ERROR : There are already the maximum number of fruits allowed on a gameboard\n");
        else{
            coordinate_t newFruitCoordinate;
            do{
                newFruitCoordinate.x = 1+rand()%(COLUMNS-2); //x∈[1,COLUMNS-2]
                newFruitCoordinate.y= 1+rand()%(ROWS-2); //x∈[1,ROWS-2]
            } while(isIntersecting(gameState,newFruitCoordinate));
        //update game state
        int ret;
        /* place snake on map */
        if((ret=pthread_mutex_lock(gameStateLock))<0)
            perror("ERROR : mutex lock failed");
        memcpy(&gameState->fruits[fruitIndex],&newFruitCoordinate,sizeof(coordinate_t)); //update fruit
        if((ret=pthread_mutex_unlock(gameStateLock))<0)
          perror("ERROR : mutex unlock failed");
        }
}
snake_t* makeSnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,int snakeIndex){
    //find 2 consecutive points which are not intersecting with fruits or snakes
    coordinate_t coordinate;//coordinate 
    coordinate_t coordinate2;//coordinate 
    do{
        coordinate.x= 5+rand()%(COLUMNS-9); //x∈[5,COLUMNS-5]
        coordinate.y= 5+(rand()%(ROWS-9)); //x∈[5,ROWS-5]
        /* create second coordinate one step to the left based of the first coordinate */
        coordinate2.x=coordinate.x-1;
        coordinate2.y=coordinate.y;
    } while(isIntersecting(gameState,coordinate)||isIntersecting(gameState,coordinate2));
    snake_t* snake = (snake_t*)malloc(sizeof(snake_t)); //create snake struct
    memset(snake,0,sizeof(snake_t)); //set it to 0;
    snake->head.x=coordinate.x;//set head 
    snake->head.y=coordinate.y;//set head 
    snake->tail[0].x=coordinate2.x;//set tail
    snake->tail[0].y=coordinate2.y;//set tail
    snake->length=2;//set length
    int ret;
    /* place snake on map */
    if((ret=pthread_mutex_lock(gameStateLock))<0)
        perror("ERROR : mutex lock failed");
    memcpy(&gameState->snakes[snakeIndex],snake,sizeof(snake_t)); //copy contents to snake on map
    if((ret=pthread_mutex_unlock(gameStateLock))<0)
        perror("ERROR : mutex unlock failed");
    return snake; 
}
//returns true if coordinate intersects a fruit or snake, false otherwise
bool isIntersecting(gamestate_t* gameState,coordinate_t coordinate){
    for(int i=0;i<MAX_FRUITS;i++)
        if(((gameState->fruits[i]).x==coordinate.x) &&((gameState->fruits[i]).y==coordinate.y))
            return true;//intersects
    for(int i=0;i<(gameState->numberOfPlayers);i++){
        //check if coordinate intersects snake head
        if((gameState->snakes[i].head.x==coordinate.x)&&(gameState->snakes[i].head.y==coordinate.y))
            return true;//intersects
        //check if coordinate intersects snake tail
        for(int j=0;j<((gameState->snakes[i].length)-1);j++)
            if(gameState->snakes[i].tail[j].x==coordinate.x&&gameState->snakes[i].tail[j].y==coordinate.y)
                return true; //coordinate intersects with some point in tail
    }
    return false; //otherwise
}
snake_t* createSnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,int playerNumber, coordinate_t coordinate){
    int ret;
    //lock mutex
    if((ret=pthread_mutex_lock(gameStateLock))<0)
        perror("ERROR : mutex lock failed");
    snake_t* snake = malloc(sizeof(snake_t));
    memset(snake,0,sizeof(snake_t));
    int check = TRUE;
    snake->head= coordinate;
    snake->tail[0].x=(coordinate.x)-1;
    snake->tail[0].y=(coordinate.y);
    memcpy(&(gameState->snakes)[playerNumber-1],snake,sizeof(snake_t));
    //unlock mutex
    if((ret=pthread_mutex_unlock(gameStateLock))<0)
        perror("ERROR : mutex unlock failed");
    return snake;
}

void moveSnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,snake_t* snake, int snakeIndex,char snakeDirection){
    int ret=0;//error handling
    bool hitBoundary=false;
    coordinate_t newHead;
    switch(snakeDirection) {
        case DIR_UP:{
            printf("Player %d attempted to move up\n",snakeIndex+1);
            newHead.y = snake->head.y-1;
            newHead.x = snake->head.x;
            break;
        }
        case DIR_DOWN:{
             printf("Player %d attempted to move down\n",snakeIndex+1);
            newHead.y= snake->head.y+1;
            newHead.x = snake->head.x;
            break;
        }
        case DIR_LEFT:{
            printf("Player %d attempted to move left\n",snakeIndex+1);
            newHead.x= snake->head.x-1;
            newHead.y = snake->head.y;
             break;
        }
        case DIR_RIGHT:{
            printf("Player %d attempted to move right\n",snakeIndex+1);
            newHead.x = snake->head.x+1;
            newHead.y = snake->head.y;
            break;
        }
    }
    /* check if we hit boundary */
    if(newHead.x<=0||newHead.y<=0||newHead.x>=COLUMNS-1||newHead.y>=ROWS-1){
        printf("Player %d hit a boundary\n",snakeIndex+1);
        hitBoundary=true; // we hit the boundary
    }
    
    if(hitBoundary){
        snake->isDead=true; //snake died :( RIP
    }
    else
        nextMove(gameStateLock,gameState,snakeIndex,snake,newHead,snakeDirection);
    /* update snake on map */
    if((ret=pthread_mutex_lock(gameStateLock))<0)
        perror("ERROR : mutex lock failed");
        memcpy(&gameState->snakes[snakeIndex],snake,sizeof(snake_t)); //update snake
    if((ret=pthread_mutex_unlock(gameStateLock))<0)
          perror("ERROR : mutex unlock failed");
}
void nextMove(pthread_mutex_t* gameStateLock,gamestate_t* gameState,int snakeIndex, snake_t* snake,coordinate_t newHead,char snakeDirection){
    int ret;
    int fruitIndex = -1;
    bool goneBackwards = false;
    if(snake->length>=2&&newHead.x==snake->tail[0].x&&newHead.y==snake->tail[0].y){
        printf("Player %d gone backwards\n",snakeIndex+1);
        goneBackwards=true;
        switch (snakeDirection){
            case DIR_UP:{
                moveSnake(gameStateLock,gameState,snake,snakeIndex,DIR_DOWN);//a recursive call to move
                break;
            }
            case DIR_DOWN:{
                moveSnake(gameStateLock,gameState,snake,snakeIndex,DIR_UP);//a recursive call to move
                break;
            }
             case DIR_RIGHT:{
                moveSnake(gameStateLock,gameState,snake,snakeIndex,DIR_LEFT);//a recursive call to move
                break;
            }
             case DIR_LEFT:{
                moveSnake(gameStateLock,gameState,snake,snakeIndex,DIR_RIGHT);//a recursive call to move
                break;
            }
        }
    }
    /* move only if not gone backwards */
    if(!goneBackwards){
        //check if it collides with another snake
        if(isCollidingWithASnake(gameState,newHead)){
            printf("Player %d hit some snake (or itself)\n",snakeIndex+1);
            snake->isDead=true; //snake dies :( RIP
            /* check if it collides with fruit */
        }

        else if((fruitIndex=isCollidingWithAFruit(gameStateLock,gameState,newHead))!=(-1)){
            coordinate_t newTail[MAX_SNAKE_LEN-1];//new tail
            memset(newTail,0,sizeof(coordinate_t)*(MAX_SNAKE_LEN-1));//init to 0
            newTail[0].x= snake->head.x;//make new tail include old head 
            newTail[0].y==snake->head.y;//make new tail include old head 
            int snakeLength = snake->length;
            for(int i=1;i<snakeLength;i++){
                newTail[i].x=snake->tail[i-1].x; //update tail to include old tail
                newTail[i].y=snake->tail[i-1].y; //update tail to include old tai
            }
            snake->head.x=newHead.x;//set new head
            snake->head.y=newHead.y;//set new head
            snake->length=(snake->length)+1;//increase snake length
            if((ret=pthread_mutex_lock(gameStateLock))<0)
                perror("ERROR : mutex lock failed");
            gameState->fruits[fruitIndex].x=0;//reset fruit
            gameState->fruits[fruitIndex].y=0;//reset fruit
            if((ret=pthread_mutex_unlock(gameStateLock))<0)
                perror("ERROR : mutex unlock failed");

            addFruit(gameStateLock,gameState);//add a random fruit 
            memcpy(snake->tail,newTail,sizeof(coordinate_t)*(MAX_SNAKE_LEN-1)); //copy new tail to snake
            printf("Player %d ate a fruit\n",snakeIndex+1);
            printf("Player %d length %d\n",snakeIndex+1,snake->length);

        }

        /* no collision with fruit & snake */
        else{
            coordinate_t newTail[MAX_SNAKE_LEN-1];//new tail
            memset(newTail,0,sizeof(coordinate_t)*(MAX_SNAKE_LEN-1));//init to 0
            printf("head  x%d\n",snake->head.x);
            printf("head y %d\n",snake->head.y);
            newTail[0].x=snake->head.x;//make new tail include old head 
            newTail[0].y=snake->head.y;//make new tail include old head  
            for(int i=1;i<(snake->length)-1;i++){
                newTail[i].x=snake->tail[i-1].x; //update tail to include old tail without last coordinate
                newTail[i].y=snake->tail[i-1].y;
            }   
            snake->head.x=newHead.x;//set new head
            snake->head.y=newHead.y;//set new head
            memcpy(snake->tail,newTail,sizeof(coordinate_t)*(MAX_SNAKE_LEN-1)); //copy new tail to snake
        }
    } 
}
bool isCollidingWithASnake(gamestate_t* gameState,coordinate_t coordinate){
    for(int i=0;i<(gameState->numberOfPlayers);i++){
        //check if coordinate intersects snake head
        if((gameState->snakes[i].head.x==coordinate.x)&&(gameState->snakes[i].head.y==coordinate.y))
            return true;//intersects
        //check if coordinate intersects snake tail
        for(int j=0;j<((gameState->snakes[i].length)-1);j++)
            if(gameState->snakes[i].tail[j].x==coordinate.x&&gameState->snakes[i].tail[j].y==coordinate.y)
                return true; //coordinate intersects with some point in tail
    }
    return false;
}
/* return fruit index of intersection, other -1 if no fruit intersects */
int isCollidingWithAFruit(pthread_mutex_t* gameStateLock,gamestate_t* gameState,coordinate_t coordinate){
    for(int i=0;i<MAX_FRUITS;i++)
        if(((gameState->fruits[i]).x==coordinate.x) &&((gameState->fruits[i]).y==coordinate.y))
            return i;//intersects
    return -1;//otherwise
}
