#include <stdio.h>
#include <stdlib.h> //Eg. int atoi(const char *str) Conver the string pointed to, by the argument str to an integer (type int).
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  //For next two .h file
#include <sys/socket.h> // include a number of definitions of structures needed for sockects. Eg. defines the sockaddr structure
#include <netinet/in.h> // contains constants and structures needed for internet domain address. Eg. sockaddr_in
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct _potato_t potato_t;
struct _potato_t{
    int hops;
    int count;
    int path[513];
};

typedef struct _player_t player_t;
struct _player_t{
    int player_id;
    int player_fd;
    int server_fd;
    int player_num;
    char player_port[6];
    char ip_address[65];
    int left_player_id;
    int right_player_id;
    char left_player_port[6];
    char right_player_port[6];
    char left_ip_address[65];
    char right_ip_address[65];
};
