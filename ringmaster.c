#include "potato.h"

/*
Run ./ringmaster <port_num> <num_players> <num_hops>
*/


int main(int argc, char *argv[])
{
  if(argc!=4){
    printf("Syntax Error: ringmaster <port_num> <num_players> <num_hops>\n");
    exit(EXIT_FAILURE);
  }
  for (int i = 1; i < 4; i++) {
    int j = 0;
    while (argv[i][j] != '\0') {
      if (argv[i][j] < '0' || argv[i][j] > '9') {
        printf("Please input numbers\n");
        exit(EXIT_FAILURE);
      }
      j++;
    }
  }
  
  int check_port_num = atoi(argv[1]);
  const char * port_num = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  if(check_port_num < 1025 || check_port_num > 65535){
    printf("Port number error: between 1025 - 65535 \n");
    exit(EXIT_FAILURE);
  }
  if(num_players < 1){
    printf("Player number error: greater than one\n");
    exit(EXIT_FAILURE);
  }
  if(num_hops < 0 || num_hops > 512){
    printf("Hop number error: greater than or equal to zero and less than or equal to 512");
    exit(EXIT_FAILURE);
  }
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", num_players);
  printf("Hops = %d\n", num_hops);

  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port_num, &host_info, &host_info_list);
  if (status != 0) {
    printf("get address info for host failed\n");
    exit(EXIT_FAILURE);
  }

  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    perror("create socket failed:");
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    perror("bind failed:");
    exit(EXIT_FAILURE);
  }

  status = listen(socket_fd, num_players);
  if (status == -1) {
    perror("listen failed:");
    exit(EXIT_FAILURE);
  }

  //Connect players with ringmaster.
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  player_t myplayers[num_players];
  for(int i=0; i< num_players; i++){
    memset(&myplayers[i], 0, sizeof(myplayers[i]));
  }
  for(int i=0; i< num_players; i++){
    myplayers[i].player_id = i;
    myplayers[i].player_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    struct sockaddr_in sin;
    memcpy(&sin, &socket_addr, sizeof(sin));
    char * temp = inet_ntoa(sin.sin_addr);
    memcpy(myplayers[i].ip_address, temp, strlen(temp)+1);
    if (myplayers[i].player_fd == -1) {
      printf("accept from players failed\n");
      exit(EXIT_FAILURE);
    }
    player_t player_buffer;
    status = recv(myplayers[i].player_fd, &player_buffer, sizeof(player_buffer), 0);
    if(status == -1){
      printf("recv player information failed\n");
      exit(EXIT_FAILURE);     
    }
    snprintf(myplayers[i].player_port, 6, "%s", player_buffer.player_port);
    //printf("player[%d] IP address is %s.\n", i, myplayers[i].ip_address);
    //printf("player[%d] port number is %s.\n", i, myplayers[i].player_port);
  }

  //Set players' information to create a ring.
  for(int i = 0; i < num_players; i++){
    if(i == 0){
      myplayers[i].left_player_id = myplayers[num_players - 1].player_id;
      strcpy(myplayers[i].left_player_port, myplayers[num_players - 1].player_port);
      strcpy(myplayers[i].left_ip_address, myplayers[num_players - 1].ip_address);
      myplayers[i].right_player_id = myplayers[i + 1].player_id;
      strcpy(myplayers[i].right_player_port, myplayers[i + 1].player_port);
      strcpy(myplayers[i].right_ip_address, myplayers[i + 1].ip_address);
    }
    else if(i == num_players - 1){
      myplayers[i].left_player_id = myplayers[i - 1].player_id;
      strcpy(myplayers[i].left_player_port, myplayers[i - 1].player_port);
      strcpy(myplayers[i].left_ip_address, myplayers[i - 1].ip_address);
      myplayers[i].right_player_id = myplayers[0].player_id;
      strcpy(myplayers[i].right_player_port, myplayers[0].player_port);
      strcpy(myplayers[i].right_ip_address, myplayers[0].ip_address);      
    }
    else{
      myplayers[i].left_player_id = myplayers[i - 1].player_id;
      strcpy(myplayers[i].left_player_port, myplayers[i - 1].player_port);
      strcpy(myplayers[i].left_ip_address, myplayers[i - 1].ip_address);
      myplayers[i].right_player_id = myplayers[i + 1].player_id;
      strcpy(myplayers[i].right_player_port, myplayers[i + 1].player_port);
      strcpy(myplayers[i].right_ip_address, myplayers[i + 1].ip_address);
    }
    myplayers[i].server_fd = socket_fd;
    myplayers[i].player_num = num_players;
  }
  
  //Send set up information to players.
  player_t player_info_buffer;
  for(int i = 0; i < num_players; i++){
    //For debug.
    // printf("player id is %d, player fd is %d, server fd is %d, port number is %s, ip address is %s.\n", myplayers[i].player_id, myplayers[i].player_fd,myplayers[i].server_fd, myplayers[i].player_port, myplayers[i].ip_address);
    // printf("player[%d] left id is %d, right id is %d.\n", i, myplayers[i].left_player_id,myplayers[i].right_player_id);
    // printf("player[%d] left port is %s, right port is %s.\n", i, myplayers[i].left_player_port, myplayers[i].right_player_port);
    // printf("player[%d] left ip address is %s, right ip address is %s.\n", i, myplayers[i].left_ip_address, myplayers[i].right_ip_address);
    memset(&player_info_buffer, 0, sizeof(player_info_buffer));
    memcpy(&player_info_buffer, &myplayers[i], sizeof(player_info_buffer));
    status = send(myplayers[i].player_fd, &player_info_buffer, sizeof(player_info_buffer), 0);
    if (status == -1) {
      printf("ringmaster send set to players failed.\n");
      exit(EXIT_FAILURE);
    }
    printf("Player %d is ready to play\n",i);
  }

  //Check the all players are ready for game.
  for(int i=0; i< num_players; i++){
     player_t player_ready_buffer;
     memset(&player_ready_buffer, 0, sizeof(player_ready_buffer));
     status = recv(myplayers[i].player_fd, &player_ready_buffer, sizeof(player_ready_buffer), MSG_WAITALL);
     if(status == -1){
       printf("ringmaster recv ready signal failed\n");
       exit(EXIT_FAILURE);     
     }
  }

  potato_t hot_potato;
  memset(&hot_potato, 0, sizeof(hot_potato));
  hot_potato.hops = num_hops;
  hot_potato.count = 0;
  fd_set readfds;
  int max_fd = -1;

  srand((unsigned int)time(NULL));
  int random = rand() % num_players;
  if(hot_potato.hops != 0){
    printf("Ready to start the game, sending potato to player %d\n", random);
  }
  status = send(myplayers[random].player_fd, &hot_potato, sizeof(hot_potato), 0);
  if(status == -1){
    printf("send potato failed, cannot start game\n");
  }

  potato_t potato_buffer;
  memset(&potato_buffer, 0, sizeof(potato_buffer));

  while(1){
    max_fd = -1;
    FD_ZERO(&readfds);
    for(int i = 0; i < num_players; i++){
      int temp_fd = myplayers[i].player_fd;
      FD_SET(temp_fd, &readfds);
      if(temp_fd > max_fd){
        max_fd = temp_fd;
      }
    }
    status = select(max_fd+1, &readfds, NULL, NULL, NULL);
    if(status == -1){
      printf("ringmaster select failed\n");
    }
    
    for (int i = 0; i < num_players; i++){
      
      if (FD_ISSET(myplayers[i].player_fd, &readfds)){
	      int flag = 0;      
    	  status = recv(myplayers[i].player_fd, &flag, sizeof(flag), 0);
        flag = 1;
        status = send(myplayers[i].player_fd, &flag, sizeof(flag), 0);
	      status = recv(myplayers[i].player_fd, &potato_buffer, sizeof(potato_buffer),0);
        potato_t player_potato_copy;
        memcpy(&player_potato_copy, &potato_buffer, sizeof(player_potato_copy));
        if(status == -1){
          printf("ringmaster receive end potato from players failed\n");
          exit(EXIT_FAILURE);     
        }
        for(int i = 0; i < num_players; i++){
          player_potato_copy.hops = -1;
          status = send(myplayers[i].player_fd, &player_potato_copy, sizeof(player_potato_copy), 0);
        }
         for(int i = 0; i < num_players; i++){
          shutdown(myplayers[i].player_fd, SHUT_RDWR);
        }
        // else{
        //   printf("Receive end potato from player[%d]\n", potato_buffer.path[num_hops-1]);
        // }
        if(potato_buffer.hops == 0){     
          printf("Trace of potato:\n");
          for(int i = 0; i< num_hops; i++){
             if(i == num_hops - 1){
               printf("%d\n", potato_buffer.path[i]);
             }
             else{
               printf("%d, ", potato_buffer.path[i]);
             }
          }
        }
          freeaddrinfo(host_info_list);
          close(socket_fd);
          return EXIT_SUCCESS;
      }
    } 
  }

  freeaddrinfo(host_info_list);
  close(socket_fd);

  return EXIT_SUCCESS;
}
