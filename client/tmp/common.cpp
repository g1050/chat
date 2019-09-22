#include "common.h"

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
    printf("进入\n");
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
            Recv_file(buf);
            break;
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


void *do_epoll(void *arg)
{
    int epfd;
    struct epoll_event events[EPOLLEVENTS];
    char buf[MAXSIZE];
    int ret;
    
    epfd = epoll_create(FDSIZE);
    add_event(epfd,listenfd,EPOLLIN);

    //获取已经准备好的描述符事件,主循环
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


