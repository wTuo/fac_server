#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <stdint.h>

#include <event2/event.h>

#include <map>

#include <mysql/mysql.h>

#include <boost/multiprecision/cpp_int.hpp>
#include "./pubkey/ecc.h"
#include "./bn/bn_boost.h"

const unsigned short DEFAULT_PORT=8888;
const int BACKLOG = 100; // Connection queue size for accept()
extern const char* SERVER_IP;
static struct timeval const FIVE_SECONDS = {5,0};

#define MAX_INFO_SIZE 1024

static unsigned int const CMD_1	= 0x19824576;
static unsigned int const CMD_2	= 0x89852035;
static unsigned int const CMD_3	= 0x59350463;
static unsigned int const CMD_4	= 0x28451104;
static unsigned int const CMD_5	= 0x49752002;



enum io_status {READY_FOR_RECEV_GX=0, READY_FOR_SEND_GY, READY_FOR_RECEV_INFO, RECEV_INFO_COMPLETE, RECEV_INFO_ERROR};

extern struct event_base *main_base_;

void write_handler(evutil_socket_t fd, short what_ev, void* arg);
void read_handler(evutil_socket_t fd, short what_ev,void* arg);
void base_event_handler(evutil_socket_t fd, short what_ev, void* arg);

extern EPoint  G;
extern Curve sm2;




class session_info{
 public:
  int session_fd;
  int status;
  struct event* read_ev;
  struct event* write_ev;
  cpp_int d;
  EPoint kG_from_client;
  cpp_int key;
  session_info(){};
  session_info(int fd);
};
extern std::map<int, session_info> sessions_;


extern MYSQL* db_conn,mysql;


int close_conn(int fd);
int new_conn(int fd);

class tcp_server
{
 public:
  static tcp_server& create_server()
  {
    static tcp_server server;
    return server;
  }
  int init();
  int conn_to_db();
  int run();

 private:
  tcp_server();
  unsigned short port_;
  int server_fd_;
  struct event* listen_ev_;



};
