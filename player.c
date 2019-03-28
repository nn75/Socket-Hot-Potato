#include "potato.h"

/*
Run ./player <machine_name> <port_num>
*/
int find_max_fd(int l, int r, int s){
  if(l>r){
    if(l>s){
      return l;  
    } 
    else{
      return s;  
    }
  }
  else{
    if(r<s) {
      return s;   
    }
    else {
      return r;
    }      
  }
}


//Throw potato to ringmaster, left player or right player.
int throwpotato(int left_fd, int right_fd, int socket_fd, potato_t * player_potato, player_t * player_set, int random){
  int flag = 0;
  potato_t player_potato_copy;
  memcpy(&player_potato_copy, player_potato, sizeof(player_potato_copy));
  //printf("%d\n",player_potato_copy.hops);
  int status = 0;
  player_potato_copy.path[player_potato_copy.count] = player_set->player_id;
  //printf("hops = %d, path[%d] = %d\n", player_potato_copy.hops, player_potato_copy.count, player_potato_copy.path[player_potato_copy.count] );
  player_potato_copy.hops--;
  player_potato_copy.count++;
  if(player_potato_copy.hops == 0){
    printf("I’m it\n");
    status = send(socket_fd, &flag, sizeof(flag), 0);
    status = recv(socket_fd, &flag, sizeof(flag), 0);
    status = send(socket_fd, &player_potato_copy, sizeof(player_potato_copy), 0);
    if(status == -1){
      printf("player[%d] send potato to master failed.\n", player_set->player_id);
    }
    player_potato->hops = -1;
    shutdown(left_fd, SHUT_RDWR);
    shutdown(right_fd, SHUT_RDWR);
    flag = 1;
  }
  else{
    if(random == 0){
      printf("Sending potato to %d\n",player_set->left_player_id);
      status = send(left_fd,  &player_potato_copy, sizeof(player_potato_copy), 0);
      if(status == -1){
        printf("player[%d] send potato to left failed.\n", player_set->player_id);
      }
    }
    else{
      printf("Sending potato to %d\n",player_set->right_player_id);
      status = send(right_fd, &player_potato_copy, sizeof(player_potato_copy), 0);
      if(status == -1){
        printf("player[%d] send potato to right failed.\n", player_set->player_id);
      }
    }
  }
  if(flag == 1)
 	return -1;
  return 1;
}


int main(int argc, char *argv[])
{
  if(argc!=3){
    printf("Syntax Error: player <machine_name> <port_num>\n");
    exit(EXIT_FAILURE);
  }
  char * ringmaster_name = argv[1];
  int check_port_num = atoi(argv[2]);
  const char * port_num = argv[2];

  if(check_port_num < 1025 || check_port_num > 65535){
    printf("Port number error: between 1025 - 65535 \n");
  }
  // printf("Ringmaster Name = %s\n", ringmaster_name);
  // printf("Port Number  = %d\n", check_port_num);

  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = argv[1];
  
  if (argc < 2) {
      printf("Syntax: client <hostname>\n");
      exit(EXIT_FAILURE);
  }

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port_num, &host_info, &host_info_list);
  if (status != 0) {
    printf("Error: cannot get address info for host\n");
    exit(EXIT_FAILURE);
  }

  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    perror("create socket failed:");
    exit(EXIT_FAILURE);
  }

  //printf("Connecting to %s, on port %s ...\n",hostname,port_num);
  
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    perror("connect failed:");
    exit(EXIT_FAILURE);
  }

  //Get player's port number and hostname.
  player_t player_to_master;
  memset(&player_to_master, 0, sizeof(player_to_master));
  struct addrinfo player_info;
  struct addrinfo *player_info_list;
  const char *player_name = NULL;
  int player_fd = 0;
  memset(&player_info, 0, sizeof(player_info));
  player_info.ai_family   = AF_UNSPEC;
  player_info.ai_socktype = SOCK_STREAM;
  player_info.ai_flags    = AI_PASSIVE;

  //Search a port number for the player.
  int port_search = 35000;
  int port_found = 0;
  while(port_search < 65535){
    char port[6];
    snprintf(port, 6, "%d", port_search);
    status = getaddrinfo(player_name, port, &player_info, &player_info_list);
    if (status != 0) {
      printf("get address info for the player failed\n");
      exit(EXIT_FAILURE);
    }
    player_fd = socket(player_info_list->ai_family, player_info_list->ai_socktype, player_info_list->ai_protocol);
    if (player_fd == -1) {
      perror("create player's socket failed:\n");
      exit(EXIT_FAILURE);
    }
    int yes = 1;
    status = setsockopt(player_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(player_fd, player_info_list->ai_addr, player_info_list->ai_addrlen);
    if (status == -1) {
      freeaddrinfo(player_info_list);
      port_search++;
    }
    else if (status == 0){
      port_found = port_search;
      break;
    }
  }
  if(port_search == 65535){
    printf("Cannot find proper port number\n");
    exit(EXIT_FAILURE);
  }
  status = listen(player_fd, 5);
  if (status == -1) {
    perror("player listen failed:");
    exit(EXIT_FAILURE);
  }
  else{
    snprintf(player_to_master.player_port, 6, "%d", port_found);
    //printf("%s\n", player_to_master.player_port);
  }

  //Senf the player's port number information to ringmaster.
  status = send(socket_fd, &player_to_master, sizeof(player_to_master), 0);
  if (status == -1) {
    printf("send port information to ringmaster failed\n");
    exit(EXIT_FAILURE);
  }

  //Receive ring set up information from server
	player_t player_set;
  status = recv(socket_fd,&player_set,sizeof(player_set),0); 

  //Join player with its left and right, connect to left neighbor and accept from right neighbor.
  int left_fd = 0;
  struct addrinfo p_info;
  struct addrinfo *p_info_list;
  memset(&p_info, 0, sizeof(p_info));
  p_info.ai_family   = AF_UNSPEC;
  p_info.ai_socktype = SOCK_STREAM;
  p_info.ai_flags    = AI_PASSIVE;
  status = getaddrinfo(player_set.left_ip_address,player_set.left_player_port, &p_info, &p_info_list);
  if (status != 0){
    printf("get address info for left player failed");
    exit(EXIT_FAILURE);
  }
  left_fd = socket(p_info_list->ai_family, 
			p_info_list->ai_socktype, 
			p_info_list->ai_protocol);
  if (left_fd == -1){
    perror("create socket for left failed:");
    exit(EXIT_FAILURE);
  }
  int right_fd = 0;  
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  
  if((player_set.player_id)%2 == 0){                    //if the id is even, connect then accept.
    status = connect(left_fd, p_info_list->ai_addr, p_info_list->ai_addrlen);  
    if (status == -1) {
      perror("connect to left socket failed:");
      exit(EXIT_FAILURE);
    }
    right_fd = accept(player_fd,  (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (right_fd == -1){
      perror("accept right socket failed:");
      exit(EXIT_FAILURE);
    }  
  }
  else{                                                //if the id is odd, accept then connect.
    right_fd = accept(player_fd,  (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (right_fd == -1){
      perror("accept right socket failed:");
      exit(EXIT_FAILURE);
    }
    status = connect(left_fd, p_info_list->ai_addr, p_info_list->ai_addrlen);  
    if (status == -1) {
      perror("connect to left socket failed:");
      exit(EXIT_FAILURE);
    }
  }
  printf("Connected as player %d out of %d total players\n", player_set.player_id, player_set.player_num);

  
  //For test: check if right and left are joined.
  // player_t test_player;
  // test_player.player_id = player_set.player_id;
  // printf("%d\n", test_player.player_id);
  // send(right_fd, &test_player, sizeof(test_player), 0);

  // player_t r_test_player;
  // recv(left_fd, &r_test_player, sizeof(r_test_player), 0);
  // printf("Received: %d\n", r_test_player.player_id);

  //Send ready signal to ringmaster.
  status = send(socket_fd, &player_set, sizeof(player_set), 0);
  if (status == -1) {
    printf("send ready signal to ringmaster failed\n");
    exit(EXIT_FAILURE);
  }

  fd_set readfds;

  int max_fd = -1;
    
  potato_t get_buffer;
  potato_t player_potato;
  memset(&get_buffer, 0, sizeof(get_buffer));
  memset(&player_potato, 0, sizeof(player_potato));
  
  srand( (unsigned int) time(NULL) + player_set.player_id);
  
  while(1){
    max_fd = -1;
    FD_ZERO(&readfds);
    
    FD_SET(left_fd, &readfds);
    FD_SET(right_fd, &readfds);
    FD_SET(socket_fd, &readfds);
    max_fd = find_max_fd(left_fd,right_fd,socket_fd);

    int random = rand() % 2;
    select(max_fd + 1, &readfds, NULL, NULL, NULL);

    //Potato from where?
    if(FD_ISSET(left_fd, &readfds)){
      status = recv(left_fd,&get_buffer,sizeof(get_buffer),MSG_WAITALL);
      if(status == 0){
        break;
      }
      if(status == -1){
        printf("player[%d] receive potato from left failed.\n", player_set.player_id);
      }
      if(get_buffer.hops == -1){
        break;
      }
      if(throwpotato(left_fd,right_fd,socket_fd,&get_buffer, &player_set,random) == -1){
        break;
      }
		}
		if(FD_ISSET(right_fd, &readfds)){
			status = recv(right_fd,&get_buffer,sizeof(get_buffer),MSG_WAITALL);
      if(status == 0){
        break;
      }
      if(status == -1){
        printf("player[%d] receive potato from right failed.\n", player_set.player_id);
      }
      if(get_buffer.hops == -1){
        break;
      }
      if(throwpotato(left_fd,right_fd,socket_fd,&get_buffer,&player_set,random) == -1){
        break;
      }
		}
    if(FD_ISSET(socket_fd, &readfds)){
      status = recv(socket_fd,&get_buffer,sizeof(get_buffer),MSG_WAITALL);
      if(status == 0){
        break;
      }
      if(status == -1){
        printf("player[%d] receive potato from master failed.\n", player_set.player_id);
      }
      if(get_buffer.hops == -1){
        break;
      }
      //printf("Player[%d] receive potato from master, hops: %d, count: %d\n", player_set.player_id, get_buffer.hops, get_buffer.count);
      if(get_buffer.hops == 0){
         status = send(socket_fd, &get_buffer, sizeof(get_buffer), 0);
         if(status == -1){
             printf("player[%d] send potato to master failed.\n", player_set.player_id);
             exit(EXIT_FAILURE);
         }
         break;
      }
      if(throwpotato(left_fd,right_fd,socket_fd,&get_buffer,&player_set,random) == -1){
        break;
      }        
    }
  }

  close(socket_fd);
  close(left_fd);
  close(right_fd);
  close(player_fd);
  freeaddrinfo(host_info_list);
  freeaddrinfo(player_info_list);
  freeaddrinfo(p_info_list);


  return EXIT_SUCCESS;
}
