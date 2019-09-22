
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <cassert>

#define LISTENQ 12                    //连接请求队列的最大长度
#define SERV_ADDRESS  "192.168.43.35"
#define EPOLL_SIZE    5000
#define SERV_PORT    4507    
#define EPOLLEVENTS 100
#define MAXSIZE     1024
#define FDSIZE      1000
#define REGISTER 1
#define LOGIN        2


pthread_mutex_t lock_login;
pthread_cond_t    cond_login;
pthread_mutex_t lock_modpwd;
pthread_cond_t cond_modpwd;
pthread_mutex_t lock_show;
pthread_cond_t cond_show;
pthread_mutex_t lock_msg;
pthread_cond_t cond_msg;
pthread_mutex_t lock_grp;
pthread_cond_t cond_grp;
pthread_mutex_t lock_grpmsg;
pthread_cond_t cond_grpmsg;

const char *file_friend = "friend.txt";
const char *file_friend_tmp = "friendtmp.txt";
const char *file_offmsg = "offmsg.txt";
const char *file_offmsg_tmp = "offmsgtmp.txt";
const char *file_message = "message.data";
const char *file_message_tmp  = "messagetmp.data";
const char *file_group = "group.txt";//群请求
const char *file_group_tmp = "grouptmp.txt";
const char *file_offgrpmsg = "groupmsg.txt";
const char *file_offgrpmsg_tmp = "groupmsgtmp.txt";


int flag_login  = 0;
int flag_question = 0;
int listenfd;
int loginuser;
int talkuser = -1;
int talkgrp = 0;
int length;
long long offset = 0;


typedef struct task
{
    void *(*process) (void*arg);//函数指针，指向任务
    void *arg;                  //回调函数的参数
    struct task *next;          //指向下一个任务
}task_t;

typedef struct thread_pool
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;

    task_t* queue_head;

    int shutdown;
    pthread_t *threadid;

    int max_thread_num;
    int cur_queue_size;
}thread_pool;

static thread_pool* pool = NULL;

//密保问题结构体
typedef struct security_question
{
    int flag;//3密保问题
    int username;
    char  phonenum[18];
    char home[256];
    char newpasswd[30];
}security_question;

//消息结构体
typedef struct message
{
    int flag;
    char mg[256];
}message;

//账户信息结构体
typedef struct  account
{
    int flag;//1注册 2登录
    int  username;
    char passwd[30];
    char nickname[30];
}account;

typedef struct union_account
{
    int flag;//3密保问题
    int username;
    char passwd[30];
    char nickname[30];
    char  phonenum[18];
    char home[256];
}union_account;

typedef struct chat_message
{
    int flag;
    char mg[256];
    int sender;
    int receiver;
}chart_message;

typedef struct group
{
    int flag;
    int id;//群号
    char name[30];
    int owner;
    int mg1;
    int mg2;
    int mg3;
}gruop;

typedef struct file
{
    int flag;
    int sender;
    int receiver;
    long long  size;
    long long pos;
    char name[100];
    char data[800];
}file;;

void Offline_message_persist(char *buf);
 void Group_notice();

//线程池中加入任务
int pool_add_task(void*(*process)(void*arg),void *arg)
{
    int ret = 0;
    //构造一个新的任务小节点
    task_t *new_task = (task_t *)malloc(sizeof(task_t));
    new_task->process = process;
    new_task->arg = arg;
    new_task->next = NULL;//初始化为空
    pthread_mutex_lock(&(pool->queue_lock));

    //将任务加入到等待队列中
    task_t * cur_task = pool->queue_head;
    if(cur_task != NULL)
    {
        while(cur_task->next != NULL)
            cur_task = cur_task->next;
        cur_task->next = new_task;
    }
    else
    {
        pool->queue_head = new_task;
    }
    //连接新任务
    
    if(pool->queue_head == NULL)
    {
        ret = -1;
        printf("func pool_add_worker err:%d",ret);
        return ret;
    }

    pool->cur_queue_size++;
    pthread_mutex_unlock(&(pool->queue_lock));

    pthread_cond_signal(&(pool->queue_cond));
    return ret;
}
//任务接口函数，子线程统一调用这个函数，这个函数检查任务队列中的任务
void *thread_routine(void *arg)
{
    /* printf("线程[0x%lx]加入线程池\n",pthread_self()); */

    //死循环保持所有线程
    while(1)
    {
        pthread_mutex_lock (&(pool->queue_lock));//给线程加锁
        
        //无任务状态和不销毁时，线程阻塞等待
        while(pool->cur_queue_size == 0 && pool->shutdown != 1)
        {
            /* printf("线程[0x%lx]正在等待\n",pthread_self()); */
            pthread_cond_wait(&(pool->queue_cond),&(pool->queue_lock));//基于条件变量阻塞 
        }
        
        if(pool->shutdown)//线程池要销毁
        {
            //先解锁后结束
            pthread_mutex_unlock(&(pool->queue_lock));
            /* printf("线程[0x%lx]将要销毁\n",pthread_self()); */
            pthread_exit(NULL);
        }

        /* printf("线程[0x%lx]将要执行任务\n",pthread_self()); */

        //断言函数
        assert(pool->cur_queue_size != 0);
        assert(pool->queue_head != NULL);

        //任务队列长度-1,取出队首节点就是要执行的任务
        pool->cur_queue_size--;
        task_t * cur_task = pool->queue_head;
        pool->queue_head = cur_task->next;
        //解锁
        pthread_mutex_unlock(&(pool->queue_lock));
        (*(cur_task->process))(cur_task->arg);//函数指针
        free(cur_task);
        cur_task = NULL;
    }
    printf("线程[0x%lx]异常退出线程池\n",pthread_self());
}

//线程池初始化
//pool表示指向头结点的一个指针，max_thread_num表示线程池中最大的线程数
void threadpool_init(int max_thread_num)
{
    //对头指针的初始化
    pool = (thread_pool*)malloc(sizeof(thread_pool));

    pthread_mutex_init(&(pool->queue_lock),NULL);
    pthread_cond_init(&(pool->queue_cond),NULL);
    pool->queue_head = NULL;
    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;
    pool->shutdown = 0;
    pool->threadid = (pthread_t *)malloc(max_thread_num*sizeof(pthread_t));
    //thread_t 表示线程的标示
    
    for(int i = 0;i<max_thread_num;i++)
    {
        pthread_create(&(pool->threadid[i]),NULL,thread_routine,NULL);
    }
    
}

//销毁线程池中的所有线程，清空任务队列
//pool指向头节点的指针
int threadpool_destroy()
{
    int ret = 0;
    if(pool->shutdown)
    {
        ret = -1;
        printf("多次销毁线程池:%d\n",ret);
        return ret;//防止多次调用
    }
    pool->shutdown = 1;
    
    //唤醒所有线程,以便于销毁
    pthread_cond_broadcast(&(pool->queue_cond));

    for(int i = 0;i<pool->max_thread_num;i++)
    {
        //阻塞主线程，结束这些线程
        pthread_join((pool->threadid[i]),NULL);
    }
    free(pool->threadid);

    //销毁任务队列
    task_t *cur = NULL;//辅助指针遍历整个链表
    while(pool->queue_head != NULL)
    {
        cur = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(cur);
    }

    //销毁互斥锁和条件变量
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_cond));
    free(pool);
    pool  = NULL;
    return ret;

}
//自定义错误处理函数
void my_err(const char *s,int line)
{
    fprintf(stderr,"line:%d",line);
    perror(s);
    exit(1);
}

int Send_cmessage(int flag ,int receiver,char *buf)
{
    char s[MAXSIZE];
    chat_message msg;
    msg.flag = flag;
    strcpy(msg.mg,buf);
    msg.sender = loginuser;
    msg.receiver = receiver;
    memcpy(s,&msg,sizeof(msg));
    int nwrite = send(listenfd,s,MAXSIZE,0);
    if(nwrite == -1){
        my_err("发送失败",__LINE__);
        return 0;
    }
    return 1;
}

int Send_srmessage(int flag ,int receiver,int sender,char *buf)
{
    char s[MAXSIZE];
    chat_message msg;
    msg.flag = flag;
    strcpy(msg.mg,buf);
    msg.sender = sender;
    msg.receiver = receiver;
    memcpy(s,&msg,sizeof(msg));
    int nwrite = send(listenfd,s,MAXSIZE,0);
    if(nwrite == -1){
        my_err("发送失败",__LINE__);
    }
    return 1;
}

int Send_message(int flag ,char *buf)
{
    char s[MAXSIZE];
    message msg;
    msg.flag = flag;
    strcpy(msg.mg,buf);
    memcpy(s,&msg,sizeof(msg));
    int nwrite = send(listenfd,s,MAXSIZE,0);
    if(nwrite == -1){
        my_err("发送失败",__LINE__);
    }
    return 1;
}

void Print_welcome(char *buf)
{
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    printf("->  %s  -<\n",msg.mg);
}

void Print_cmessage(char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    if(talkuser == msg.sender || talkuser == 0)
        printf("                                                              [%d]:%s\n",msg.sender,msg.mg);
    else{
        printf("                                                              收到一条好友消息\n");
        Offline_message_persist(buf);
    } 
}

void Print_grpmsg(char *buf)
{
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    if(strcmp(msg.mg,"oover")){
        printf("                                                              %s",msg.mg);
    }
    else{//唤醒阻塞状态的好友列表
        pthread_mutex_lock(&lock_grpmsg);
        pthread_cond_signal(&cond_grpmsg);
        pthread_mutex_unlock(&lock_grpmsg);
    }
}

void Print_qmessage(char *buf)
{
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    if(strcmp(msg.mg,"oover")){
        printf("                                                              %s",msg.mg);
    }
    else{//唤醒阻塞状态的好友列表
        pthread_mutex_lock(&lock_msg);
        pthread_cond_signal(&cond_msg);
        pthread_mutex_unlock(&lock_msg);
    }
}


void Print_friend(char *buf)
{
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    if(strcmp(msg.mg,"over")){
        printf("%s",msg.mg);
    }
    else{//唤醒阻塞状态的好友列表
        pthread_mutex_lock(&lock_show);
        pthread_cond_signal(&cond_show);
        pthread_mutex_unlock(&lock_show);
    }
    // printf("按[ENTER]返回上层\n");
    // getchar();
}

void Print_group(char *buf)
{
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    if(strcmp(msg.mg,"over")){
        printf("%s",msg.mg);
    }
    else{//唤醒阻塞状态的好友列表
        pthread_mutex_lock(&lock_grp);
        pthread_cond_signal(&cond_grp);
        pthread_mutex_unlock(&lock_grp);
    }
}
//创建套接字并进行连接,返回新创建的套接字文件描述符
int socket_connect(char *ip,int port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);//获取新创建的套接字的文件描述符
    if(listenfd == -1) {
        my_err("套接字创建失败",__LINE__);
    }
    bzero(&servaddr,sizeof(servaddr)); //初始化套接字地址结构体
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    
    if(connect(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0){
        my_err("连接失败",__LINE__);
    }
    else{
            printf("\n");
            printf("\033[1m\033[44;37m  ****************************************** \033[0m\n");
            printf("\033[1m\033[44;37m  *           成功连接至服务器              *\033[0m\n");
            printf("\033[1m\033[44;37m  ****************************************** \033[0m\n");
    } 
    return listenfd;
}

//添加事件
void add_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);//事件注册函数
    return ;
}


void Add_friend()
{
    int username;
    system("clear");
    printf("请输入要添加的好友账号:");
    scanf("%d",&username);
     while(getchar() != '\n');//清空缓冲区
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(6,username,"") == 1){
       printf("                                                              好友请求发送成功\n");
   }
    return ;
}

void Private_chat()
{
    int  receiver;
    system("clear");
    printf("请输入要私聊的好友账号:");
    scanf("%d",&receiver);
    getchar();
    talkuser = receiver;
    char buf[256];
    printf("[Quit退出聊天]->:");
    
    fgets(buf,256,stdin);
    do{
        Send_cmessage(7,receiver,buf);
        printf("[Quit退出聊天]->:");
        fgets(buf,256,stdin);
    }while(strcmp(buf,"Quit\n"));
    talkuser = -1;
    //如果发送来的消息sender和talkuser一致 或为0  (未处于聊天界面) 直接打印
    //如果不相等存入未读消息
    return ;
}

//删除friend.data文件中前cnt个数据
void Delete_fdnotice(int cnt,const char *filename,const char *tmp)
{
    //printf("cnt = %d\n",cnt);
    char buf[MAXSIZE];
    int fd ,fd_tmp;
    if((fd = open(filename,O_RDWR)) == -1){
        my_err(filename,__LINE__);
     }
       if((fd_tmp = open(tmp,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
        my_err(tmp,__LINE__);
    }

     //向后移动cnt位
    for(int i = 0;i<cnt;i++){
        read(fd,buf,sizeof(chat_message));
    }

    while(read(fd,buf,sizeof(chat_message)) != 0){
        if(write(fd_tmp,buf,sizeof(chat_message))  == -1){
            my_err(tmp,__LINE__);
        }
    }

    
    remove(filename);
    rename(tmp,filename);
    close(fd);
}

void Friend_notice()
{
    int fd;
    chat_message msg;
    if((fd = open(file_friend,O_RDWR)) == -1){
        return ;
        //my_err(file_friend,__LINE__);
    }

    char buf[MAXSIZE];
    int cnt = 0;
    while( read(fd,buf,sizeof(msg))  != 0){
        cnt++;
        memcpy(&msg,buf,sizeof(msg));
        printf("%s ",msg.mg);
        char ch;
        
        scanf("%c",&ch);
        getchar();
        if(ch == 'Y' || ch == 'y'){
            if( Send_cmessage(8,msg.sender,"Y") == 1){
            printf("                                                              同意好友请求发送成功\n");
   }
        }
       else{
           if( Send_cmessage(8,msg.sender,"N") == 1)
            printf("                                                              拒绝好友请求发送成功\n");
       }
        
       printf("                                                              是否继续读取下一条好友通知[Y/N]? ");
       char next;
       
       scanf("%c",&next);
       getchar();
       if(next == 'N' || 'n' == next){
          Delete_fdnotice(cnt,file_friend,file_friend_tmp);
           return ;
       } 
    
    }

    Delete_fdnotice(cnt,file_friend,file_friend_tmp);
    printf("                                                              所有消息处理完毕\n");
    close(fd);
}

void Show_friend()
{
    pthread_mutex_lock(&lock_show);
    printf("==================\n");
    printf("账号    状态\n");
    message msg;
    msg.flag = 9;
    char str[15];
   if( Send_cmessage(9,loginuser,"") == 1){
        ;//printf("查看好友列表请求发送成功\n");
    }
    //阻塞等待好友加载完成
    pthread_cond_wait(&cond_show,&lock_show);
    printf("==================\n");
    // printf("按[ENTER]返回上层\n");
    // getchar();
    pthread_mutex_unlock(&lock_show);
    return ;
}

//删除offmsg.data文件中前cnt个数据
void Delete_offmsg(int cnt)
{
    //printf("cnt = %d\n",cnt);
    char buf[MAXSIZE];
    int fd ,fd_tmp;
    if((fd = open(file_offmsg,O_RDWR)) == -1){
        my_err(file_offmsg,__LINE__);
     }
       if((fd_tmp = open(file_offmsg_tmp,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
        my_err(file_offmsg_tmp,__LINE__);
    }

     //向后移动cnt位
    for(int i = 0;i<cnt;i++){
        read(fd,buf,sizeof(chat_message));
    }

    while(read(fd,buf,sizeof(chat_message)) != 0){
        if(write(fd_tmp,buf,sizeof(chat_message))  == -1){
            my_err(file_offmsg_tmp,__LINE__);
        }
    }

    remove(file_offmsg);
    rename(file_offmsg_tmp,file_offmsg);
    close(fd);
    close(fd_tmp);
    return ;
}

void Read_offmsg()
{
    int fd;
    chat_message msg;
    if((fd = open(file_offmsg,O_RDWR)) == -1){
        my_err(file_offmsg,__LINE__);
    }

    char buf[MAXSIZE];
    int cnt = 0;
    while( read(fd,buf,sizeof(msg))  != 0){
        cnt++;
        memcpy(&msg,buf,sizeof(msg));
        if(msg.flag == 7 || msg.flag == 14){//聊天信息需要包装一下
            printf("                                                             [%d]: %s\n",msg.sender,msg.mg);
        }
        else printf("                                                              %s ",msg.mg);
        
       printf("                                                              是否继续读取下一条好友通知[Y/N]? ");
       char next;
       getchar();
       scanf("%c",&next);
       if(next == 'N' || 'n' == next){
          Delete_offmsg(cnt);
           return ;
       } 
       
    
    }

    Delete_offmsg(cnt);
    printf("                                                              所有消息处理完毕\n");
    close(fd);
    
}

void Delete_friend()
{
    int username;
    system("clear");
    printf("请输入你要删除的好友账号:");
    scanf("%d",&username);
     while(getchar() != '\n');//清空缓冲区
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(11,username,"") == 1){
        printf("删除好友请求发送成功\n");
   }
    return ;
}

void Shield_friend()
{
    int username;
    system("clear");
    printf("请输入你要屏蔽的好友账号:");
    scanf("%d",&username);
     while(getchar() != '\n');//清空缓冲区
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(12,username,"") == 1){
        printf("屏蔽好友请求发送成功\n");
    }
    return ;
}

void Relieve_friend()
{
     int username;
    system("clear");
    printf("请输入你要解除屏蔽的好友账号:");
    scanf("%d",&username);
     while(getchar() != '\n');//清空缓冲区
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(13,username,"") == 1){
        printf("解除屏蔽请求发送成功\n");
    }
    return ;
}

void Query_message()
{
    int username;
    system("clear");
    printf("请输入你要查询的好友账号:");
    scanf("%d",&username);
    
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(15,username,"") == 1){
        printf("查询聊天记录请求发送成功\n");
    }

    //阻塞该线程等待聊天记录加载完毕
    pthread_mutex_lock(&lock_msg);
    pthread_cond_wait(&cond_msg,&lock_msg);
    pthread_mutex_unlock(&lock_msg);
    return ;
}

void Manage_friend()
{
    system("clear");
    int choice = -1;
    while(choice){
        printf("[1]我的好友\n");
        printf("[2]私聊好友\n");
        printf("[3]添加好友\n");
        printf("[4]好友通知\n");
        printf("[5]好友消息\n");
        printf("[6]删除好友\n");
        printf("[7]屏蔽好友\n");
        printf("[8]解除屏蔽\n");
        printf("[9]查看聊天记录\n");
        printf("[0]退出\n");
        
        printf("\n请输入你的选择:");
        scanf("%d",&choice);
         while(getchar() != '\n');//清空缓冲区
        switch(choice){
            case 1:
                Show_friend();
                break;
            case 2:
                Private_chat();
                break;
            case 3:
                Add_friend();
                break;
            case 4:
                Friend_notice();
                break;
            case 5:
                Read_offmsg();
                break;
            case 6:
                Delete_friend();
                break;
            case 7:
                Shield_friend();
                break;
            case 8:
                Relieve_friend();
                break;
            case 9:
                Query_message();
                break;
            case 0:
                break;
        }
    }
    return ;
}

//发起创建群的请求 flag = 17
void Creat_grp()
{
    char second[30];
    char str[MAXSIZE];
    group grp; 
    memset(str,0,sizeof(str));
    printf("请输入群名:");
    scanf("%s",grp.name);
    grp.owner = loginuser;//群主就是注册群的账号
    grp.flag = 17;
    memcpy(str,&grp,sizeof(grp));
    if(send(listenfd,str,MAXSIZE,0) == -1){
        my_err("注册群时时发送出错",__LINE__);
    }    
    return ;
}

 void Manage_myrp()
 {
     system("clear");
    int choice = -1;
    while(choice){
        printf("[1]加群申请\n");
        printf("[2]踢人\n");
        printf("[3]管理员设置\n");
        printf("[4]解散群\n");

        printf("[0]退出\n");
        
        printf("\n请输入你的选择:");
        scanf("%d",&choice);
         while(getchar() != '\n');//清空缓冲区
        switch(choice){
            case 1:
                Group_notice();
                break;
            // case 2:
            //     Private_chat();
            //     break;
            // case 3:
            //     Add_friend();
            //     break;
            // case 4:
            //     Friend_notice();
            //     break;
            case 0:
                break;
        }
    }
    return ;
 }

//申请加入群,flag = 18
 void Add_grp()
 {
     int id;
    system("clear");
    printf("请输入要添加的群账号:");
    scanf("%d",&id);
   if( Send_cmessage(18,id,"") == 1){//第二个参数receiver在这就是要添加的群的群号
       printf("                                                              申请加群请求发送成功\n");
    }

    return ;
 }

 void Group_notice()
 {
    int fd;
    char s[30];
    char buffs[MAXSIZE];
    chat_message msg;//s 请求  r 处理者 m 群号
    chat_message msgfs; //s群号 r请求者 m Y/N
    if((fd = open(file_group,O_RDWR)) == -1){
        my_err(file_group,__LINE__);
    }

    setbuf(stdin,NULL);
    char buf[MAXSIZE];
    int cnt = 0;
    while( read(fd,buf,sizeof(msg))  != 0){
        cnt++;
        memcpy(&msg,buf,sizeof(msg));
        printf("                                                              [%d]请求加入[%s] Y?N",msg.sender,msg.mg);
        char ch;
        scanf("%c",&ch);
        getchar();
        printf("\n\nch = %c\n",ch);
        if(ch == 'Y' || ch == 'y'){
            sprintf(s,"%c%d",'Y',msg.receiver);
            if( Send_srmessage(19,msg.sender,atoi(msg.mg),"Y") == 1){
            printf("                                                              同意入群请求发送成功\n");
          }
        }
       else{
           sprintf(s,"%c%d",'N',msg.receiver);
           if( Send_srmessage(19,msg.sender,atoi(msg.mg),"N") == 1)
            printf("                                                              拒绝入群请求发送成功\n");
       }
        
       printf("                                                              是否继续读取下一条好友通知[Y/N]? ");
       char next;
       scanf("%c",&next);
       getchar();
       printf("\n\nnext = %c\n",next);
       if(next == 'N' || 'n' == next){
          Delete_fdnotice(cnt,file_group,file_group_tmp);
           return ;
       } 
        
    }
    close(fd);
    Delete_fdnotice(cnt,file_group,file_group_tmp);//传入新旧文件名字
    printf("                                                              所有消息处理完毕\n");
 }

 void Read_offgrpmsg()
 {
     int fd;
    chat_message msg;
    
    if((fd = open(file_offgrpmsg,O_RDWR)) == -1){
        return ;
    }

    char buf[MAXSIZE];
    int cnt = 0;
    while( read(fd,buf,sizeof(msg))  != 0){
        cnt++;
        memcpy(&msg,buf,sizeof(msg));
        printf("群消息->%s ",msg.mg);
        
       printf("                                                              是否继续读取下一条好友通知[Y/N]? ");
       char next;
       getchar();
       scanf("%c",&next);
       if(next == 'N' || 'n' == next){
          Delete_fdnotice(cnt,file_offgrpmsg,file_offgrpmsg_tmp);
           return ;
       } 
    
    }
    Delete_fdnotice(cnt,file_offgrpmsg,file_offgrpmsg_tmp);
    printf("                                                              所有消息处理完毕\n");
    close(fd);
 }


void Show_group()
{
    pthread_mutex_lock(&lock_grp);
    printf("=============================\n");
    printf("%5s %5s %5s %5s %5s\n","群号","群主","管理员1","管理员2","管理员3");
    message msg;
    msg.flag = 9;
    char str[15];
   if( Send_cmessage(22,loginuser,"") == 1){//s 自己 r 自己 msg 空
        ;//printf("查看群列表请求发送成功\n");
    }
    //阻塞等待好友加载完成
    pthread_cond_wait(&cond_grp,&lock_grp);
    printf("=============================\n");
    pthread_mutex_unlock(&lock_grp);
    return ;
}
void Broadcast_msg()
{
     int  id;
    system("clear");
    printf("请输入要发消息的群账号:");
    scanf("%d",&id);
     while(getchar() != '\n');//清空缓冲区
    talkgrp = id;
    char buf[256];
    printf("[Quit退出聊天]->:");
    fgets(buf,256,stdin);
    //printf("buf = %s\n",buf);
    do{
        Send_cmessage(21,id,buf);//sdender 发送者 receiver 群号 msg 消息
        printf("[Quit退出聊天]->:");
        fgets(buf,256,stdin);
        //printf("buf = %s\n",buf);
    }while(strcmp(buf,"Quit\n"));
    talkgrp = 0;
    //如果发送来的消息sender和talkuser一致 或为0  (未处于聊天界面) 直接打印
    //如果不相等存入未读消息
    return ;
}

void Delete_grp()
{
    int username;
    system("clear");
    printf("请输入你要退出的群账号:");
    scanf("%d",&username);
    
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(23,username,"") == 1){
        printf("                                                              退群请求发送成功\n");
   }
    return ;
}

//查询群聊天记录flag = 25
void Query_grpmsg()
{
    int username;
    system("clear");
    printf("请输入你要查询的群账号:");
    scanf("%d",&username);
     while(getchar() != '\n');//清空缓冲区
    char str[15];
    sprintf(str,"%d",username);
   if( Send_cmessage(25,username,"") == 1){
        printf("                                                              查询群聊天记录请求发送成功\n");
    }

    //阻塞该线程等待聊天记录加载完毕
    pthread_mutex_lock(&lock_grpmsg);
    pthread_cond_wait(&cond_grpmsg,&lock_grpmsg);
    pthread_mutex_unlock(&lock_grpmsg);
    return ;
}

void Manage_group()
{
    system("clear");
    int choice = -1;
    while(choice){
        printf("[1]已加入群组\n");
        printf("[2]发送群消息\n");
        printf("[3]添加群组\n");
        printf("[4]加群请求\n");
        printf("[5]群消息\n");
        printf("[6]退出群组\n");
        //printf("[7]管理我的群\n");
        printf("[8]新建群组\n");
        printf("[9]查看群聊天记录\n");
        printf("[0]退出\n");
        
        printf("\n请输入你的选择:");
        scanf("%d",&choice);
         while(getchar() != '\n');//清空缓冲区
        switch(choice){
            case 1:
                Show_group();
                break;
            case 2://flag = 21 发送群消息
                Broadcast_msg();
                break;
            case 3:
                Add_grp();
                break;
            case 4:
                Group_notice();
                break;
            case 5:
                Read_offgrpmsg();
                break;
            case 6:
                Delete_grp();
                break;
            case 7:
                //Manage_myrp();
                break;
            case 8:
                Creat_grp();
                break;
            case 9:
                Query_grpmsg();
                break;
            case 0:
                break;
        }
    }
    return ;
}

int  Send_file_persist(int receiver,char filename[100])
{
    file f;
    char buf[MAXSIZE];

    f.receiver = receiver;
    f.sender = loginuser;
    f.flag = 24;
    f.pos = offset;
    strcpy(f.name,filename);
    printf("sizeof(file) = %d\n",sizeof(file));
    int fd = open(filename,O_RDONLY);

    //读文件发结构体 read 返回值为0表示读到文件尾
    while( (f.size = pread(fd,f.data,sizeof(f.data),offset) ) != 0){
        f.pos = offset;
        printf("offset = %lld\n",offset);
        printf("f.size = %lld\n",f.size);
        //sleep(1);
        
        memcpy(buf,&f,sizeof(f));
        if(send(listenfd,buf,MAXSIZE,0) == -1){
            printf("发送异常\n");
            perror("send");
            return 0;
        } 
        offset += f.size;
    }

    close(fd);

    return 1;
}

void Manage_file()
{
    int receiver;
    printf("请输入要发送的好友");
    scanf("%d",&receiver);
    char filename[100];
    printf("请输入要发送的文件的路径");
    scanf("%s",filename);
    if(Send_file_persist(receiver,filename)){
        printf("                                                              文件发送成功\n");
        sleep(2);
        getchar();
    }else{
        printf("                                                              文件发送出错\n");
        sleep(2);
        getchar();
    }
}

void Main_display()
{
    int choice = -1;
    while(choice){
        system("clear");
        //好友管理
        //群管理
        //注销登录
        //发送文件
        printf("[1]好友管理\n");
        printf("[2]群组管理\n");
        printf("[3]文件传输\n");
        printf("[0]退出系统\n");
        
        //printf("length = %d\n",length);
        printf("\n请输入你的选择:");
        scanf("%d",&choice);
         while(getchar() != '\n');//清空缓冲区
        switch(choice){
            case 1:
                Manage_friend();
                break;
            case 2:
                Manage_group();
                break;
            case 3:
                Manage_file();
                break;
            case 0:
                exit(0);
                break;
        }
    }
    return ;    
}

int Account_veri(char *buf)
{
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    pthread_mutex_lock(&lock_login);
    if(!strcmp(msg.mg,"Y")){
        system("clear");
        printf(">- 登录成功 -<\n");
        flag_login = 1;
    }
    else if(!strcmp(msg.mg,"R")){//重复登录
        printf(">- 重复登录 -<\n");
    }
    else{
        printf("-> 账号或密码错误 -<\n");
    }
        pthread_cond_signal(&cond_login);//阻塞等待验证完成
        pthread_mutex_unlock(&lock_login);
}

int Question_veri(char *buf)
{
    message msg;
    pthread_mutex_lock(&lock_modpwd);
    memcpy(&msg,buf,sizeof(msg));
    if(!strcmp(msg.mg,"Y")){
        flag_question = 1;
    }
    pthread_cond_signal(&cond_modpwd);
    pthread_mutex_unlock(&lock_modpwd);
}

void Question_send(char *buf)
{
    account ac;
    message msg;
    memcpy(&msg,buf,sizeof(msg));
    usleep(500);
     security_question sq;
     sq.flag = 3;
     sq.username = atoi(msg.mg);
     printf("sq_username = %d\n",sq.username);
    printf("请输入你的手机号:");
    scanf("%s",sq.phonenum);
    printf("你的家乡是:");
    scanf("%s",sq.home);
    printf("请输入新密码:");

    char str2[MAXSIZE];
    memcpy(str2,&sq,sizeof(sq));
   
    if(send(listenfd,str2,MAXSIZE,0) == -1){
        my_err("注册账户时发送出错",__LINE__);
    }else printf("                                                              密保问题发送成功\n");
}
void Passwd_modify(int username)
{
    puts("修改密码");
    char str[MAXSIZE];
    account ac;
    ac.flag = 5;
    ac.username = username;
     //printf("账号是:%d\n",ac.username);
    printf("请输入新密码");
    scanf("%s",ac.passwd);
    memcpy(str,&ac,sizeof(ac));
     if(send(listenfd,str,MAXSIZE,0) == -1){
        my_err("修改密码时发送出错",__LINE__);
    }else printf("                                                              修改密码请求发送成功\n");

}
void  Add_friend_request(char *buf)
{
    int fd;
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    //将好友请求存入文件中
    if((fd = open(file_friend,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
        my_err(file_friend,__LINE__);
    }
    if(write(fd,buf,sizeof(chat_message)) == -1){
        my_err(file_friend,__LINE__);
    }
    close(fd);
    // printf("%s",msg.mg);
    // char r ;
    // scanf("%c",&r);
    // if(r == 'Y' || r == 'y'){
    //     printf("同意请求\n");
    // }else printf("不同意\n");

    // getchar();
}

void Offline_message_persist(char *buf)
{
    int fd;
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    //将好友请求存入文件中
    if((fd = open(file_offmsg,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
        my_err(file_offmsg,__LINE__);
    }
    if(write(fd,buf,sizeof(chat_message)) == -1){
        my_err(file_offmsg,__LINE__);
    }
    close(fd);
}

void Offline_gspmsg_persist(char *buf)
{
    int fd;
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    //将好友请求存入文件中
    if((fd = open(file_offgrpmsg,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
        my_err(file_offgrpmsg,__LINE__);
    }
    if(write(fd,buf,sizeof(chat_message)) == -1){
        my_err(file_offgrpmsg,__LINE__);
    }
    close(fd);
}

//先存入文件中，后面手动读取
void Add_group_request(char *buf)//chat_message类型
{
    int fd;
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    //将好友请求存入文件中
    if((fd = open(file_group,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
        my_err(file_group,__LINE__);
    }
    if(write(fd,buf,sizeof(chat_message)) == -1){
        my_err(file_group,__LINE__);
    }
    close(fd);
}


void *Recv_file(void *buf)
{
    buf = (char*)buf;
    int fd;
    file f;
    memcpy(&f,buf,sizeof(f));

    //如果文件不存在创建文件
    if((fd = open(f.name,O_RDWR | O_CREAT ,0600 )) == -1){
            my_err(f.name,__LINE__);
        }
    
    
    int ret = pwrite(fd,f.data,f.size,f.pos);

    //printf("ret = %d\n",ret);

    // printf("name = %s\n",f.name);
    // printf("receiver = %d\n",f.receiver);
    // printf("sender = %d\n",f.sender);
    // printf("size = %d\n",f.size);
    free(buf);
    close(fd);
}

void Handle_grpmsg(char *buf)
{//发送的消息 sender 群号 receiver 接受者 msg 群消息
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    if(msg.sender = talkgrp || msg.sender == 0){//和当前聊天群一致直接打印
        printf("                                                              %s\n",msg.mg);
    }
    //否则存储到离线消息中
    else {
        printf("                                                              收到一条群消息\n");
        Offline_gspmsg_persist(buf);
    }
    return ;
}

int empty_file(const char *name)
{
    int fd  = open(name,O_RDONLY);
    char ch;
    read(fd,&ch,sizeof(char));
    if(ch == EOF)
        return 1;
    else return 0;
}

//处理读请求的事件
void do_read(int epollfd,int fd,int sockfd,char *buf)//fd表示待处理事件的描述符
{
    
    //printf("处理读事件\n");
    int ret;
    ret = recv(fd,buf,MAXSIZE,MSG_WAITALL);
    if(ret == -1){
        my_err("读事件处理错误",__LINE__);
        close(fd);
    }
    else if(ret == 0){
        fprintf(stderr,"服务器关闭\n");
        close(fd);
        exit(1);
    }

    int choice;
    memcpy(&choice,buf,4);
    
    if(choice == 18) printf("                                                              收到一条入群请求\n");
    else if(choice == 6) printf("                                                              收到一条好友请求\n");
    else if(choice == 10) printf("                                                              收到一条好友消息\n");
    
    //printf("choice = %d\n",choice);
    
    switch(choice)
    {
        case 0:
            Print_welcome(buf);
            break;
        case 2:
            Account_veri(buf);
            break;
        case 3:
            Question_send(buf);
            break;
        case 4:
            Question_veri(buf);
            break;
        case 6://添加好友请求
            Add_friend_request(buf);
            break;
        case 7://打印消息
            Print_cmessage(buf);
            break;
        case 9://好友状态
            Print_friend(buf);
            break;
        case 10:
            Offline_message_persist(buf);
            break;
        case 14://存储离线聊天消息
            Offline_message_persist(buf);
            break;
        case 16://打印聊天记录
            Print_qmessage(buf);
            break;
        case 18://群主或者管理员处理加群请求
            Add_group_request(buf);
            break;
        case 20://处理群消息
            Handle_grpmsg(buf);
            break;
        case 22://接受群信息
            Print_group(buf);
            break;
        case 24://接受文件
        {
            char *buf2 = (char*)malloc(MAXSIZE);
            memcpy(buf2,buf,MAXSIZE);
            pool_add_task(Recv_file,(void *)buf2);
             break;
        }
            
            //Recv_file(buf);
           
        case 26://打印群聊天记录
            Print_grpmsg(buf);
            break;
    }
    
    

    memset(buf,0,MAXSIZE);
    return ;
}

void do_write(int epollfd,int fd,int sockfd,char *buf)
{
    printf("处理写事件\n");
    /* write(fd,"hello",5); */
    int nwrite;
    nwrite = send(fd,buf,strlen(buf),0);
    if(nwrite == -1){
        my_err("写事件处理错误",__LINE__);
        close(fd);
    }
    else{
        printf("发送消息成功\n");
    }
    memset(buf,0,MAXSIZE);
}
void  Account_register()
{
    char second[30];
    char str[MAXSIZE];
    union_account reg;
    memset(str,0,sizeof(str));
    printf("昵称:");
    scanf("%s",reg.nickname);
    int tag = 0;
    do{
            if(tag) puts("两次输入密码不一致!请重新输入");
            printf("密码:");
            scanf("%s",reg.passwd);
            printf("再次输入密码:");
            scanf("%s",second);
            tag = 1;
    }while( strcmp(second,reg.passwd));
    reg.flag = REGISTER;
    printf("[密保问题]请输入手机号:");
    scanf("%s",reg.phonenum);
    printf("[密保问题]你的家乡是:");
    scanf("%s",reg.home);
    memcpy(str,&reg,sizeof(union_account));
    if(send(listenfd,str,MAXSIZE,0) == -1){
        my_err("注册账户时发送出错",__LINE__);
    }    
  
    


    // account tmp;
    // memcpy(&tmp,str,MAXSIZE);
    // printf("标志是%d\n",tmp.flag);
    // printf("昵称是%s\n",tmp.nickname);
    // printf("密码是%s\n",tmp.passwd);
    return ;
    
}

//解决重复登录问题
void Account_login()
{
    account ac;
    char str[MAXSIZE];
    ac.flag = 2;
    printf("账号:");
    scanf("%d",&ac.username);
    loginuser = ac.username;
    printf("密码:");
    scanf("%s",ac.passwd);
    memcpy(str,&ac,sizeof(ac));
    if(send(listenfd,str,MAXSIZE,0) == -1){
        my_err("登录时发送消息出错",__LINE__);
    }

    
    pthread_mutex_lock(&lock_login);
    pthread_cond_wait(&cond_login,&lock_login);//阻塞等待验证完成
    if(flag_login == 1) 
    {
        
        Main_display();
    }
    flag_login = 0;
    pthread_mutex_unlock(&lock_login);
    

}

void Find_passwd()
{
    security_question sq;
    printf("账号:");
    scanf("%d",&sq.username);
    printf("请输入手机号:");
    scanf("%s",sq.phonenum);
    printf("请输入家乡:");
    scanf("%s",sq.home);
    printf("请输入新密码:");
    scanf("%s",sq.newpasswd);

    // printf("find pwd phone = %s\n ",sq.phonenum);
    // printf("find pwd home = %s",sq.home);

    char str2[MAXSIZE];
    sq.flag = 4;
    memcpy(str2,&sq,sizeof(sq));
    if(send(listenfd,str2,MAXSIZE,0) == -1){
        my_err("密保验证发送出错",__LINE__);
    }else printf("                                                              密保问题发送成功\n");
    sleep(1);

}
void Main_menu()
{
    int choice = -1;
    while(choice){
        usleep(100);
        printf("\033[1m\033[44;37m  [1]注册   [2]登录   [0]退出   [4]找回密码  \033[0m\n");
        scanf("%d",&choice);
        while(getchar() != '\n');//清空缓冲区
        //printf("Main_menu choice = %d\n",choice);
        switch(choice)
        {
            case 1:
                puts("注册");
                Account_register();
                break;
            case 2:
                puts("登录");
                Account_login();
                break;
            case 0:
                
                break;
            case 4:
                Find_passwd();
                break;
        }
    }
    
    return ;
}

void *do_epoll(void *arg)
{
    int epfd;
    struct epoll_event events[EPOLLEVENTS];
    char buf[MAXSIZE];
    int ret;
    
    epfd = epoll_create(FDSIZE);
    add_event(epfd,listenfd,EPOLLIN);

    //获取已经准备好的描述符事件,主循环xianchengchixianchengchi
    while(1) {
        int epoll_event_count = epoll_wait(epfd,events,EPOLLEVENTS,-1);//等待事件发生,ret表示需要处理的事件数目
        //if(epoll_event_count < 0) my_err("需要处理事件异常",__LINE__);//<0的时候出错
            for(int i = 0;i < epoll_event_count;i++ ){

                int fd = events[i].data.fd;//根据事件类型做相应的处理

                //只处理读写事件
                if(events[i].events & EPOLLIN)
                    do_read(epfd,fd,listenfd,buf);
             }
    }
    close(epfd);
    return NULL;
}
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

     threadpool_init(100);
    //主线程进入菜单
    Main_menu();

    
    
return 0;
}