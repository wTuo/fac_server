#include "mytcpserver.h"






int main(){
  tcp_server ts= tcp_server::create_server();
  if(ts.init()==-1){
    exit(1);
  }
  if(ts.conn_to_db()==-1) exit(1);
  ts.run();
  return 0;


}
