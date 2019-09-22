#include "common.h"

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
    talkuser = receiver;
    char buf[256];
    printf("[Quit退出聊天]->:");
    scanf("%s",buf);
    do{
        Send_cmessage(7,receiver,buf);
        printf("[Quit退出聊天]->:");
        scanf("%s",buf);
    }while(strcmp(buf,"Quit"));
    talkuser = -1;
    //如果发送来的消息sender和talkuser一致 或为0  (未处于聊天界面) 直接打印
    //如果不相等存入未读消息
    return ;
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
    getchar();
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
        printf("[0]注销登录\n");
        
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
