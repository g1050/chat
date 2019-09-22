
#include "common.h"
#include "pvachat.h"
#include "account.h"

//全局变量
extern MYSQL *con;
extern onlion_list_t list;

int main()
{
    Connect_Database(con);
    
    int listenfd = socket_bind(SERV_ADDRESS,SERV_PORT);//listenfd表示创建的套接字文件描述符
    if(listen(listenfd,LISTENQ) < 0)//将套接字转化为监听套接字
        my_err("监听失败\n",__LINE__);
    else printf("开始监听:%s\n",SERV_ADDRESS);
	
    //创建线程池
     threadpool_init(100);

    do_epoll(listenfd);
     
    close(listenfd);
    return 0;
 }
