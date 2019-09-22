#include "pvachat.h"

//全局变量
extern MYSQL *con;
extern onlion_list_t list;

int Add_friend_persist(int sender,int receiver)
{
    //先判断两个是不是已经请求过
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  friendship where (sender = %d and  receiver = %d)",sender,receiver);
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);
    if(row != NULL) {//已经请求过
        Send_message(getfd(sender),0,"等待对方验证中,不要重复发送好友请求\n");
        return 0;
    }

    //将两个元素插入表中
    char insert[MAXSIZE];
    sprintf(insert,"insert into friendship (sender,receiver) values('%d','%d')",sender,receiver);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }
    else {
        printf("[%d]对[%d]好友请求存入数据库\n",sender,receiver);
    }
    //判断是否在线在线直接发送
    if(getfd(receiver) < 0){// 不在先存入数据库中
        printf("[%d]不在线\n",receiver);
    }else{
        char str[256];
        sprintf(str,"[%d]请求添加您为好友是否同意?[Y/N]",sender);
         Send_srmessage(6,receiver,sender,str);
         //ifsd改为2表示已经发送
         char update[256];
        sprintf(update,"update friendship set ifsd = %d where (sender = %d and receiver = %d)",2,sender,receiver);//2已经发送
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                my_err("修改状态出错",__LINE__);
            }
    }
    
    
}

int Add_friend(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    printf("接收到的receicver = %d sender = %d\n",msg.receiver,msg.sender);
    Add_friend_persist(msg.sender,msg.receiver);
}

int Check_shield(int fd,int sender,int receiver)
{
     char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  friendship where ((sender = %d  and  receiver = %d) or (sender = %d and receiver = %d ))",
    sender,receiver,receiver,sender);
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }

    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);

    if(row == NULL){//二者不是好友关系
        char str[50];
        sprintf(str,"您和[%d]还不是好友关系",receiver);
        Send_message(fd,0,str);
        return -1;
    }

    if(atoi(row[3]) == 2) {//二者处于屏蔽状态
        return 0;
    }

    return 1;//未屏蔽可以正常通信
}

void Store_msg_persit(char *buf)
{
    int i,j;
    int flag = 0;
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    //printf("-------msg.flag = %d\n",msg.flag);
    char insert[MAXSIZE];
    int len = strlen(msg.mg);
    for(i = 0;i<len;i++){
        if(msg.mg[i] == '\''){
            flag = 1;
            printf("----- --- %c\n",msg.mg[i]);
            break;
        }
    }
    if(flag)
    {
         msg.mg[len+1] = '\0';
        for(j = len;j>i;j--){
             msg.mg[j] = msg.mg[j-1];
       }
    }
   

    // if(flag == 1)
    //     sprintf(insert,"insert into message (sender,receiver,flag,msg) values('%d','%d','%d','%s'')",msg.sender,msg.receiver,0,msg.mg);
    // else
     sprintf(insert,"insert into message (sender,receiver,flag,msg) values('%d','%d','%d','%s')",msg.sender,msg.receiver,0,msg.mg);
    
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }else 
        printf("[%d]和[%d]聊天记录成功录入数据库\n",msg.sender,msg.receiver);

    return ;
}

void Send_offreq_persist(int fd,int receiver)///离线好友请求
{
    char str[50];
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  friendship where (receiver = %d and ifsd = %d)",receiver,0);//0未发送
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        //标志位改为1并且发送
        char update[256];
        sprintf(update,"update friendship set ifsd = %d where (id = %d)",2,atoi(row[5]));//2已经发送
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                char str[256];
                int sender = atoi(row[0]);
                sprintf(str,"[%d]请求添加您为好友是否同意?[Y/N]",sender);
                Send_srmessage(6,receiver,sender,str);
                printf("[%d]对[%d]好友请求发送成功\n",sender,receiver);
            }
        
    }
}

void Send_offmsg_persist2(int recvfd,int receiver)
{
    char str[50];
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  message where (receiver = %d and flag = %d)",receiver,0);//0未发送
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        //标志位改为1并且发送
        char update[256];
        sprintf(update,"update message set flag = %d where (id = %d)",2,atoi(row[4]));//2已经发送
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                chat_message msg;
                msg.flag = 14;
                msg.sender = atoi(row[0]);
                msg.receiver = atoi(row[1]);
                strcpy(msg.mg,row[3]);
                char s[MAXSIZE];
                memcpy(s,&msg,sizeof(msg));
                Send_cmessage(14,recvfd,s);
                printf("[%d]对[%d]消息发送成功\n",msg.sender,msg.receiver);
            }
        
    }
}

void Send_offmsg_persist(int recvfd,int sender ,int receiver)
{
    int flag = 1;
    char str[50];
 

    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  message where (sender = %d and  receiver = %d and flag = %d)",sender,receiver,0);//0未发送
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        //标志位改为1并且发送
        char update[256];
        sprintf(update,"update message set flag = %d where (sender = %d and receiver = %d and id = %d)",2,sender,receiver,atoi(row[4]));//2已经发送
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                chat_message msg;
                msg.flag = 7;//msg类型被改成了7
                msg.sender = atoi(row[0]);
                msg.receiver = atoi(row[1]);
                strcpy(msg.mg,row[3]);
                char s[MAXSIZE];
                memcpy(s,&msg,sizeof(msg));
                Send_cmessage(7,recvfd,s);
                printf("[%d]对[%d]消息发送成功\n",sender,msg.receiver);
            }
        
    }

}

void Send_offmsg_persist2(int recvfd,int sender ,int receiver)
{
    int flag = 1;
    char str[50];
 

    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  message where (sender = %d and  receiver = %d and flag = %d)",sender,receiver,0);//0未发送
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        //标志位改为1并且发送
        char update[256];
        sprintf(update,"update message set flag = %d where (sender = %d and receiver = %d and id = %d)",2,sender,receiver,atoi(row[4]));//2已经发送
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                chat_message msg;
                msg.flag = 18;//msg类型被改成了7
                msg.sender = atoi(row[0]);
                msg.receiver = atoi(row[1]);
                strcpy(msg.mg,row[3]);
                char s[MAXSIZE];
                memcpy(s,&msg,sizeof(msg));
                Send_cmessage(7,recvfd,s);
                printf("[%d]对[%d]消息发送成功\n",sender,msg.receiver);
            }
        
    }
}

void Send_offmsg_persist3(int recvfd,int sender,int receiver)
{
     int flag = 1;
    char str[50];
 

    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  message where (sender = %d and  receiver = %d and flag = %d)",sender,receiver,0);//0未发送
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        //标志位改为1并且发送
        char update[256];
        sprintf(update,"update message set flag = %d where (sender = %d and receiver = %d and id = %d)",2,sender,receiver,atoi(row[4]));//2已经发送
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                chat_message msg;
                msg.flag = 20;//msg类型被改成了7
                msg.sender = atoi(row[0]);
                msg.receiver = atoi(row[1]);
                strcpy(msg.mg,row[3]);
                char s[MAXSIZE];
                memcpy(s,&msg,sizeof(msg));
                Send_cmessage(7,recvfd,s);
                printf("[%d]对[%d]消息发送成功\n",sender,msg.receiver);
            }
        
    }
}
//相较于原版函数，只是少了一个屏蔽判断
int Receive_message2(int  fd,char *buf)
{
    chat_message msg;
    memcpy (&msg,buf,sizeof(chat_message));

    //可以发送消息时优先存储在数据库中
    Store_msg_persit(buf);

    int recvfd = getfd(msg.receiver);
    if(recvfd < 0){//对方不在线需要先存入数据库中,等待上线后在发送给对方 flag = 0未发送
        printf("对方不在线\n");
    }

    else{ //对方在线直接发送从数据库中取出发送并且 设置 flag = 1已经发送
        printf("flag = %d\n",msg.flag);
        if(msg.flag == 20)
             Send_offmsg_persist3(recvfd,msg.sender,msg.receiver);
        else if(msg.flag == 18)
            Send_offmsg_persist2(recvfd,msg.sender,msg.receiver);
       
    } 

    return 0;
}

int Receive_message(int  fd,char *buf)
{
    chat_message msg;
    memcpy (&msg,buf,sizeof(chat_message));
    
    
    int ret = Check_shield(fd,msg.sender,msg.receiver);
    if(ret == 1){
        //可以发送消息时优先存储在数据库中
        Store_msg_persit(buf);

        int recvfd = getfd(msg.receiver);
        if(recvfd < 0){//对方不在线需要先存入数据库中,等待上线后在发送给对方 flag = 0未发送
            printf("对方不在线\n");
        }

        else{ //对方在线直接发送从数据库中取出发送并且 设置 flag = 1已经发送
            Send_offmsg_persist(recvfd,msg.sender,msg.receiver);
        } 
    }

    else if(ret == 0){
        printf("[%d]和[%d]处于屏蔽状态\n",msg.sender,msg.receiver);
        return 0;
    }
    
    return 0;
}

//负责操作后台数据库
int Add_friend_repisist(int fd,char *buf)
{

    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    if(!strcmp(msg.mg,"Y")){//同意请求，通知双方,数据库中flag+1
         char update[256];
        sprintf(update,"update friendship set flag = %d where (sender = %d and receiver = %d)",2,msg.receiver,msg.sender);
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                printf("[%d]和[%d]新好友关系建立\n",msg.receiver,msg.sender);
            }
          return 1;
    }
    
    else{//通知被拒绝一方,从数据库中删除此条加好友记录
        char update[256];
        sprintf(update,"delete from  friendship where (sender = %d and receiver = %d)",msg.receiver,msg.sender);
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                printf("[%d]和[%d]新好友关系从数据库移除\n",msg.receiver,msg.sender);
            }
        return 0;
        
    }
}

//负责发送通知信息 8交换sender 和reciver
void Add_friend_receipt(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char s1[50];
    char s2[50];
    if(Add_friend_repisist(fd,buf) ){//同意好友请求
        sprintf(s1,"[%d]通过了您的好友请求\n",msg.sender);
        Send_srmessage(10,msg.receiver,msg.sender,s1);//10离线消息

        sprintf(s2,"您已和[%d]成为好友\n",msg.receiver);
        Send_srmessage(10,msg.sender,msg.receiver,s2);//10离线消息
    }else{
        sprintf(s1,"[%d]拒绝了您的好友请求\n",msg.sender);
        Send_srmessage(10,msg.receiver,msg.sender,s1);
    }
}

//从数据库中删除好友关系
int Delete_friend_persist(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char update[256];
    sprintf(update,"delete from  friendship where ((sender = %d and receiver = %d) or(sender = %d and receiver = %d))",
    msg.receiver,msg.sender,msg.sender,msg.receiver);
    if(mysql_real_query(con,update,strlen(update))){
        finish_with_error(con);
    }else {
        printf("[%d]和[%d]新好友关系从数据库移除\n",msg.receiver,msg.sender);
        return 1;
    }
    return 0;
}

void Delete_friend(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char s1[50];
    char s2[50];
    if(Delete_friend_persist(fd,buf)){
        sprintf(s1,"您和[%d]已经解除好友关系\n",msg.receiver);
        Send_srmessage(10,msg.receiver,msg.sender,s1);//10离线消息

        sprintf(s2,"您已和[%d]已经解除好友\n",msg.sender);
        Send_srmessage(10,msg.sender,msg.receiver,s2);//10离线消息
    }
    else printf("删除好友失败\n");
    return ;
}

//屏蔽好友数据库shield状态改变为2
int  Shield_frinend_persist(int fd,char *buf)
{   
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char update[256];//shield = 2表示两个好友处于屏蔽状态
    sprintf(update,"update friendship set shield = 2 where ((sender = %d and receiver = %d) or(sender = %d and receiver = %d))",
    msg.receiver,msg.sender,msg.sender,msg.receiver);
    if(mysql_real_query(con,update,strlen(update))){
        finish_with_error(con);
    }else {
        printf("[%d]成功屏蔽[%d]\n",msg.sender,msg.receiver);
        return 1;
    }
    return 0;
}

//屏蔽好友
void Shield_frinend(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char s1[50];
    if(Shield_frinend_persist(fd,buf)){
        sprintf(s1,"您已经成功屏蔽[%d]\n",msg.receiver);
        Send_srmessage(10,msg.sender,msg.sender,s1);//10离线消息
    }else printf("屏蔽好友失败\n");
    return ;
}

int Relieve_friend_persist(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char update[256];//shield = 0表示两个好友处于未屏蔽状态
    sprintf(update,"update friendship set shield = 0 where ((sender = %d and receiver = %d) or(sender = %d and receiver = %d))",
    msg.receiver,msg.sender,msg.sender,msg.receiver);
    if(mysql_real_query(con,update,strlen(update))){
        finish_with_error(con);
    }else {
        printf("[%d]成功解除屏蔽[%d]\n",msg.sender,msg.receiver);
        return 1;
    }
    return 0;
}

 void Relieve_friend(int fd,char *buf)
 {
     chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char s1[50];
    if(Relieve_friend_persist(fd,buf)){
        sprintf(s1,"您已经成功解除屏蔽[%d]\n",msg.receiver);
        Send_srmessage(10,msg.sender,msg.sender,s1);//10离线消息
    }else printf("屏蔽好友失败\n");
    return ;
 }

int Show_friend_persist(int fd,char *buf)
{
    int flag = 1;
    chat_message msg;
    int username;
    char str[50];
    memcpy(&msg,buf,sizeof(msg));

    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  friendship where (sender = %d or  receiver = %d)",msg.sender,msg.receiver);
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        if(atoi(row[0]) == msg.sender){
            username = atoi(row[1]);
        }else{
            username = atoi(row[0]);
        }
        int status = getstatus(username);
        
        if(atoi(row[2]) == 2){
            if(status == 1){//将好友信息发送过去并且直接打印
                sprintf(str,"||%5d   %5s||\n",username,"在线");
                Send_message(fd,9,str);
            }else{
                sprintf(str,"||%5d   %5s||\n",username,"离线");
                Send_message(fd,9,str);
            }
        }
        
    }
    return flag;

}

 void  Show_friend(int fd,char *buf)
 {
     chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    if(Show_friend_persist(fd,buf)){
        Send_message(fd,9,"over");
        printf("[%d]查询好友成功\n",msg.sender);
    }else printf("[%d]查询好友失败\n",msg.sender);
    return ;
 }

//cmessgae.msg为空  查询对象保存在receiver中
int Query_friend_persist(int fd,char *buf)
{
    int flag = 1;
    chat_message msg;
    char str[50];
    memcpy(&msg,buf,sizeof(msg));

    //首先判断是不是好友关系
     char insert[MAXSIZE];
    sprintf(insert,"select  *from  friendship where ((sender = %d and  receiver = %d) or (sender = %d and receiver = %d))",
    msg.sender,msg.receiver,msg.receiver,msg.sender);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }
    MYSQL_RES *res2  = mysql_store_result(con);
    MYSQL_ROW  row2  = mysql_fetch_row(res2);
    if(row2 == NULL){
        Send_message(fd,0,"你们还不是好友关系");
        return 0;
    }

    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  message where ((sender = %d and  receiver = %d) or (sender = %d and receiver = %d))",
    msg.sender,msg.receiver,msg.receiver,msg.sender);
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        sprintf(str,"[%s]->[%s]:%s\n",row[0],row[1],row[3]);
        Send_message(fd,16,str);//传过去后flag = 16
        }
        
    return flag;
}

 void Query_message(int fd,char *buf)
 {
     chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    if(Query_friend_persist(fd,buf)){
        //Send_message(fd,16,"oover");
        printf("[%d]查询聊天记录成功\n",msg.sender);
    }else printf("[%d]查询聊天记录失败\n",msg.sender);
        Send_message(fd,16,"oover");
    return ;
 }

 