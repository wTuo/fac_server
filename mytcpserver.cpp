#include "mytcpserver.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <fstream>
#include <iostream>


std::map<int, session_info> sessions_;

EPoint  G;
Curve sm2;

struct event_base *main_base_;

const char* SERVER_IP="127.0.0.1";

session_info::session_info(int fd){
  session_fd=fd;
  status=READY_FOR_RECEV_GX;
  read_ev=event_new(main_base_, fd,EV_TIMEOUT|EV_READ, read_handler, NULL);
  write_ev=event_new(main_base_, fd, EV_TIMEOUT|EV_WRITE, write_handler, NULL);
  d=cpp_int("0x75ae6de8a376874d55f487b4ac50e6de0869c2bb13b3406bdd1bcc35d444cc6f");

  
}


tcp_server::tcp_server(){
  main_base_=event_base_new();
  port_= DEFAULT_PORT;
  server_fd_=-1;
  G.x=cpp_int("0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7");
  G.y=cpp_int("0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0");

  uint8_t sm2_param_a[] =
    {
      0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

  sm2.blockLen = CURVE_LEN;
  sm2.p = cppint_from_uint8(sm2_param_a, sizeof(sm2_param_a));
  sm2.a = 3;
  sm2.b = cpp_int("0x28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93");
  sm2.b = sm2.p - sm2.b;
  sm2.n = cpp_int("0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123");
  sm2.G.x = cpp_int("0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7");
  sm2.G.y = cpp_int("0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0");
}




int tcp_server::init(){
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(SERVER_IP); // Bind to all interfaces
  addr.sin_port = htons(port_);

  server_fd_= socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd_ == -1) {
    std::cerr << "socket() failed: " << strerror(errno) << std::endl;
    return -1;
  }
  evutil_make_socket_nonblocking(server_fd_);
  int ret = bind(server_fd_, (const struct sockaddr*)&addr, sizeof(addr));
  if (ret == -1) {
    std::cerr << "bind() failed: " << strerror(errno) << std::endl;
    close(server_fd_);
    return -1;
  }
  return 0;
}
int tcp_server::run(){
  int ret = listen(server_fd_, BACKLOG);
  if (ret == -1) {
    std::cerr << "listen() failed: " << strerror(errno) << std::endl;
    close(server_fd_);
    return -1;
  }
  listen_ev_=event_new(main_base_, server_fd_, EV_READ|EV_PERSIST, base_event_handler, NULL);
  event_add(listen_ev_, NULL);
  event_base_dispatch(main_base_);

  return 0;
  
}
int tcp_server::conn_to_db(){
  // mysql_init(&mysql);
  // db_conn=mysql_real_connect(&mysql,"localhost","root","",0,0,0);
  // if(db_conn==NULL){
  //   std::cout<<mysql_error(&mysql)<<std::endl;
  //   return -1;
  // }
  return 1;
}

int close_conn(int fd){
  event_free(sessions_[fd].read_ev);
  event_free(sessions_[fd].write_ev);
  sessions_.erase(fd);
  close(fd);
  return 0;
}

int new_conn(int fd){
  session_info session(fd);
  if(sessions_.find(fd)==sessions_.end()){
    sessions_.insert(std::pair<int, session_info>(fd,session));
    event_add(sessions_[fd].read_ev,&FIVE_SECONDS);
    return 0;
  }
  return -1;
}

void base_event_handler(evutil_socket_t fd, short what_ev,void* arg){
  struct sockaddr_in client_addr;
  socklen_t sin_size=sizeof(struct sockaddr_in);
  int client_fd=accept(fd, (struct sockaddr*)& client_addr, &sin_size);
  evutil_make_socket_nonblocking(client_fd);
  new_conn(client_fd);
  
}
void ext_cmd_and_len(uint8_t* buf,uint8_t* cmd,uint8_t* len){
  for(int i=0;i<=3;i++){
    cmd[i]=buf[i];
  }
  for(int i=4;i<=7;i++){
    len[i]=buf[i];
  }
}


// int check_comm(uint8_t* cmd,io_status status){
//   switch(status){
//     if()
    
//   }
  
// }

void read_handler(evutil_socket_t fd, short what_ev,void* arg){
  int client_fd=fd;
  int sess_status=sessions_[client_fd].status;
  if(what_ev&EV_TIMEOUT){
    std::cerr<<"ERROR: WRITE_HANDLER "<<sess_status<<" : 5s TIMEOUT."<<std::endl;
    close_conn(client_fd);
    return;
  }
  event_del(sessions_[client_fd].read_ev);
  uint8_t cmd[4],len[4];
  switch(sess_status){
  case READY_FOR_RECEV_GX:
    {
      uint8_t gx_buf[72];
      if(read(fd, gx_buf, 8)>0){
        ext_cmd_and_len(gx_buf,cmd,len);
        uint32_t cmd_= cmd[0] | (cmd[1] << 8) | (cmd[2] << 16) | (cmd[3] << 24);
        if(cmd_!=CMD_1){
          std::cerr<<"ERROR: READY_FOR_RECEV_GX: cmd check error."<<std::endl;
          close_conn(client_fd);
          return;
        }
        
      }
      else{
        std::cerr<<"ERROR: READY_FOR_RECEV_GX: pre-read error."<<std::endl;
        close_conn(client_fd);
        return;
      }
      if(read(fd, gx_buf+8, 64)>0){
        std::cout<<"GX RECEVED"<<std::endl;
        sessions_[client_fd].kG_from_client.x=cppint_from_uint8(gx_buf+8,32);
        sessions_[client_fd].kG_from_client.y=cppint_from_uint8(gx_buf+40,32);
        EPoint dkG=mul(sessions_[client_fd].d, sessions_[client_fd].kG_from_client, sm2);
        sessions_[client_fd].key=dkG.x;
        std::cout<<"key: "<<dkG.x<<std::endl;
        sessions_[client_fd].status=READY_FOR_SEND_GY;
      }
      else{
        std::cerr<<"ERROR: READY_FOR_RECEV_GX: read gx check error."<<std::endl;
        close_conn(client_fd);
        return;
      }
      break;
    }
  case READY_FOR_RECEV_INFO:
    {
      uint8_t pre_buf[8];
      int mes_len;
      if(read(client_fd, pre_buf, 8)>0){
        ext_cmd_and_len(pre_buf,cmd,len);
        uint32_t cmd_= cmd[0] | (cmd[1] << 8) | (cmd[2] << 16) | (cmd[3] << 24);
        uint32_t len_= cmd[0] | (cmd[1] << 8) | (cmd[2] << 16) | (cmd[3] << 24);
        mes_len=(int)len_;
        if(cmd_!=CMD_3){
          std::cerr<<"ERROR: READY_FOR_RECEV_INFO: cmd check error."<<std::endl;
          sessions_[client_fd].status=RECEV_INFO_ERROR;          
          event_add(sessions_[client_fd].write_ev,&FIVE_SECONDS);
          return;
        }
      }
      else{
        std::cerr<<"ERROR: READY_FOR_RECEV_INFO: pre-read error."<<std::endl;
        sessions_[client_fd].status=RECEV_INFO_ERROR;          
        event_add(sessions_[client_fd].write_ev,&FIVE_SECONDS);
        return;
      }
      uint8_t info_buf[MAX_INFO_SIZE];
      if(read(fd, info_buf, mes_len)>0){
        sessions_[client_fd].status=RECEV_INFO_COMPLETE;
      }
      else{
        std::cerr<<"ERROR: READY_FOR_RECEV_INFO: read divice info error."<<std::endl;
        sessions_[client_fd].status=RECEV_INFO_ERROR;          
        event_add(sessions_[client_fd].write_ev,&FIVE_SECONDS);
        return;
      }
      break; 
    }
  }
  event_add(sessions_[client_fd].write_ev,&FIVE_SECONDS);

  
}



void write_handler(evutil_socket_t fd, short what_ev, void* arg){
  int client_fd=fd;
  int sess_status=sessions_[client_fd].status;
  if(what_ev&EV_TIMEOUT){
    std::cerr<<"ERROR: WRITE_HANDLER "<<sess_status<<" : 5s TIMEOUT."<<std::endl;
    close_conn(client_fd);
    return;
  }
  event_del(sessions_[client_fd].write_ev);
  std::cout<<"write handler is working"<<std::endl;
  switch(sess_status){
  case READY_FOR_SEND_GY:
    {
      uint8_t send[72];
      uint32_t s_len=64;
      EPoint dG=mul(sessions_[client_fd].d,G,sm2);
      uint8_t dG_x[32];
      uint8_t dG_y[32];
      cppint_to_uint8(dG.x, dG_x, 32);
      cppint_to_uint8(dG.y, dG_y, 32);
      std::memcpy(send, &CMD_2, sizeof(CMD_2));
      std::memcpy(send+4, &s_len, sizeof(s_len));
      std::memcpy(send+8, dG_x, 32);
      std::memcpy(send+40, dG_y, 32);
      if(write(client_fd , send, 72)>0){
        std::cout<<"HAVING SEND GY "<<std::endl;
        sessions_[client_fd].status=READY_FOR_RECEV_INFO;
      }
      break;
    }
  case RECEV_INFO_COMPLETE:
    {
      uint8_t resp_ok[9];
      static unsigned int const resp_ok_len=1;
      std::memcpy(resp_ok, &CMD_4, sizeof(CMD_4));
      std::memcpy(resp_ok+4, &resp_ok_len, sizeof(resp_ok_len));
      resp_ok[8]=1;
      if(write(client_fd, resp_ok, 9)<=0){
        std::cerr<<"SEND RESPONSE OK ERROR."<<std::endl;        
          
      }
      std::cout<<"Interaction Complete!"<<std::endl;
      close_conn(client_fd);
      return;
      break;
      // if(write()){
      //   close(client_fd);
      // }
    }
  case RECEV_INFO_ERROR:
   {
     uint8_t resp_error[9];
     static unsigned int const resp_error_len=1;
     std::memcpy(resp_error, &CMD_4, sizeof(CMD_4));
     std::memcpy(resp_error, &resp_error_len, sizeof(resp_error_len));
     resp_error[8]=0;
     if(write(client_fd, resp_error, 9)<=0){
       std::cerr<<"SEND RESPONSE ERROR ERROR."<<std::endl;        
     }
     close_conn(client_fd);
     return;
     break;
  }

  }
  event_add(sessions_[client_fd].read_ev, &FIVE_SECONDS);
}