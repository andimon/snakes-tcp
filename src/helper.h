#include <ncurses.h>
#include <pthread.h>
#include<stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#define ROWS 20//gameboard height
#define COLUMNS 40//gameboard width
#define MAX_SNAKE_LEN 20
#define MAX_PLAYERS 20
#define WINNER_CODE -5
#define LOOSER_CODE -3
#define GAME_STATE_OVER -1 //end game
#define GAME_STATE_CONTINUE 0 //continue game 
#define MAX_FRUITS 6
#define PORT 6969
#define WINNING_LENGTH 15
#define DIR_UP 'U'
#define DIR_DOWN 'D'
#define DIR_LEFT 'L'
#define DIR_RIGHT 'R'
#define GAME_WINDOW_COLOUR 1
#define RED 2
#define GREEN 3
#define FRUIT_COLOUR 4
#define SNAKE_COLOUR_1 5
#define SNAKE_COLOUR_2 6
#define SNAKE_COLOUR_3 7
typedef struct coordinate_struct{
    int x,y;//a coordinate contains an x & y point
} coordinate_t;
typedef struct snake_struct{
    int length;
    bool isDead;//indicate if a snake is dead (collided with some propery of the map)
    coordinate_t head;//a head is a cordinate 
    coordinate_t tail[MAX_SNAKE_LEN-1];//a tail is at max 14 cordinates long
} snake_t;
typedef struct gamestate_struct{
    int numberOfPlayers;
    int playerWinner;
    coordinate_t fruits[MAX_FRUITS];//MAX_FRUIT -> max number of fruits that can appear on a gameboard
    snake_t snakes[MAX_PLAYERS]; //MAX_PLAYERS-> max number of snakes that can appear on a gameboard
} gamestate_t;
void centerText(WINDOW* win,int row,const char* str);
snake_t* makeSnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,int snakeIndex);
bool isIntersecting(gamestate_t* gameState,coordinate_t coordinate);
void addFruit(pthread_mutex_t* gameStateLock,gamestate_t* gameState);
coordinate_t* createRandomCoordinate(); //returns a freshly randomly generated point
snake_t* createSnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,int playerNumber, coordinate_t coordinate);
void destroySnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,snake_t* snake, int playerNumber);
void moveSnake(pthread_mutex_t* gameStateLock,gamestate_t* gameState,snake_t* snake, int snakeIndex,char dir);
void error(char *msg);  
void nextMove(pthread_mutex_t* gameStateLock,gamestate_t* gameState,int snakeIndex, snake_t* snake,coordinate_t newHead,char snake_direction);
bool isCollidingWithASnake(gamestate_t* gameState,coordinate_t coordinate);
int isCollidingWithAFruit(pthread_mutex_t* gameStateLock,gamestate_t* gameState,coordinate_t coordinate);
