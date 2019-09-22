#include "common.h"

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
        my_err(file_friend,__LINE__);
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
            printf("                                                              %s:[%d]\n",msg.mg,msg.sender);
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


 void Read_offgrpmsg()
 {
     int fd;
    chat_message msg;
    if((fd = open(file_offgrpmsg,O_RDWR)) == -1){
        my_err(file_offgrpmsg,__LINE__);
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

 
int  Send_file_persist(int receiver,char filename[100])
{
    file f;
    char buf[MAXSIZE];

    f.receiver = receiver;
    f.sender = loginuser;
    f.flag = 24;
    strcpy(f.name,filename);
    printf("sizeof(file) = %d\n",sizeof(file));
    int fd = open(filename,O_RDONLY);

    //读文件发结构体 read 返回值为0表示读到文件尾
    while( (f.size = read(fd,f.data,sizeof(f.data)))  != 0){
        length += f.size;
        memcpy(buf,&f,sizeof(f));
        if(send(listenfd,buf,MAXSIZE,0) == -1){
            return 0;
        } 
    }

    close(fd);

    return 1;
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


void Recv_file(char *buf)
{
    int fd;
    file f;
    memcpy(&f,buf,sizeof(f));

    //如果文件不存在创建文件
    if((fd = open(f.name,O_RDWR | O_CREAT |  O_APPEND,0600 )) == -1){
            my_err(f.name,__LINE__);
        }
    
    
    int ret = write(fd,f.data,f.size);

    //printf("ret = %d\n",ret);

    // printf("name = %s\n",f.name);
    // printf("receiver = %d\n",f.receiver);
    // printf("sender = %d\n",f.sender);
    // printf("size = %d\n",f.size);
    close(fd);
}