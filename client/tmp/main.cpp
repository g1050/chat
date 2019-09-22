#include "common.h"
#include "view.h"
#include "persist.h"

int main()
{
    setbuf(stdin,NULL);
    pthread_t thid;
    pthread_mutex_init(&lock_login,NULL);
    pthread_cond_init(&cond_login,NULL);
    pthread_mutex_init(&lock_modpwd,NULL);
    pthread_cond_init(&cond_modpwd,NULL);
    pthread_mutex_init(&lock_show,NULL);
    pthread_cond_init(&cond_show,NULL);
    pthread_mutex_init(&lock_msg,NULL);
    pthread_cond_init(&cond_msg,NULL);
    pthread_mutex_init(&lock_grpmsg,NULL);
    pthread_cond_init(&cond_grpmsg,NULL);

    char buf[MAXSIZE];
     listenfd = socket_connect(SERV_ADDRESS,SERV_PORT);


    //创建子线程专门接受消息
    if(pthread_create(&thid,NULL,do_epoll,NULL) != 0){
        my_err("创建线程失败",__LINE__);
    }

    //主线程进入菜单
    Main_menu();

    
    
return 0;
}