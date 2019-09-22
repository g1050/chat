#include "grpchat.h"
#include "pvachat.h"

int Creat_grp_persisit(group grp)
{
     char insert[MAXSIZE];
    sprintf(insert,"insert into g_roup (name,owner) values('%s','%d')",grp.name,grp.owner);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }else printf("新群成功写入数据库\n");

    //再从数据库中获取插入的主键值,即账号
    if(mysql_query(con,"select LAST_INSERT_ID()")){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);

    return atoi(row[0]);
}

//将群基本数据写入数据库中
void Creat_grp(int fd,char *buf)
{
    gruop grp;
    memcpy(&grp,buf,sizeof(grp));
    int ret = Creat_grp_persisit(grp);
    if(ret){//反馈提示客户端群注册成功以及群号
        char str2[256];
        sprintf(str2,"新群[%d]注册成功",ret);
        printf("%s\n",str2);
        Send_message(fd,0,str2);

        //将群主信息写入数据库
         char insert[MAXSIZE];
         sprintf(insert,"insert into pertogrp  (username,id,flag) values('%d','%d','%d')",grp.owner,ret,2);
        if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }
    // else {
    //     printf("[%d]对群[%d]请求存入数据库\n",msg.sender,msg.receiver);
    // }
    }else{
        my_err("注册群失败",__LINE__);
    }
}

int  Get_grpbaseinfo_persist(group &grp,int id)
{
    
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  g_roup where (id =  %d)",id );
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);
    
    if(row == NULL)//row可能为空
         return 0;
 
    grp.id = id;
    grp.owner = atoi(row[2]);
    grp.mg1 = atoi(row[3]);
    grp.mg2 = atoi(row[4]);
    grp.mg3 = atoi(row[5]);

    return 1;
}

void Add_grp(int fd,char *buf)
{//flag = 18 sender请求者 receiver 群号
    chat_message msg;
    chat_message msgfs;//用于发送请求
    char s[50];
    char buf2[MAXSIZE];//用于转换成发送格式
    memcpy(&msg,buf,sizeof(msg));
    
    //将请求加群者信息写入数据库
    //将两个元素插入表中
    char insert[MAXSIZE];
    sprintf(insert,"insert into pertogrp  (username,id) values('%d','%d')",msg.sender,msg.receiver);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }
    else {
        printf("[%d]对群[%d]请求存入数据库\n",msg.sender,msg.receiver);
    }

    //获取到群基本信息 把请求发送给群主和管理员
    group grp;
    if(Get_grpbaseinfo_persist(grp,msg.receiver)){//把请求发给群主等等 flag = 18 recv = 群主/管理员 send = 请求者 recv = 群号
        //也要判断是否处于离线状态
        msgfs.flag = 18;
        msgfs.sender = msg.sender;
        msgfs.receiver = grp.owner; 
        sprintf(s,"%d",msg.receiver);
        strcpy(msgfs.mg,s);
        memcpy(buf2,&msgfs,sizeof(msgfs));
        Receive_message2(fd,buf2);

        if(grp.mg1 > 0){
            msgfs.receiver = grp.mg1;
            memcpy(buf2,&msgfs,sizeof(msgfs));
            Receive_message2(fd,buf2);
        }
        if(grp.mg2 > 0){
            msgfs.receiver = grp.mg2;
            memcpy(buf2,&msgfs,sizeof(msgfs));
            Receive_message2(fd,buf2);
        }
        if(grp.mg3 > 0){
            msgfs.receiver = grp.mg3;
            memcpy(buf2,&msgfs,sizeof(msgfs));
            Receive_message2(fd,buf2);
        }
    }else{
        Send_message(fd,0,"该群不存在");
    }
    return ;
}

void Set_flag_persist(int username,int id)
{
    char update[256];
    sprintf(update,"update pertogrp set flag = %d where (username = %d and id = %d)",2,username,id);//2已经发送
    if(mysql_real_query(con,update,strlen(update))){
        finish_with_error(con);
    }else {
        printf("修改pertogrp的flag成功\n");
    }
    return ;
}
void Add_group_receipt(int fd,char *buf)
{//s 群号 r请求者 m Y/N
    chat_message msg;
    chat_message msgfs;
    memcpy(&msg,buf,sizeof(msg));
    char s[50];
    char buf2[MAXSIZE];
    msgfs.flag = 20;
    msgfs.sender = msg.receiver;//s 成功入群者 r 成功加群者 m 提示
    msgfs.receiver = msg.receiver;  
    printf("msg.mg = %s\n",msg.mg);
    printf("sender = %d\n",msg.sender);
    printf("receiver = %d\n",msg.receiver);
    //如果mg = "y" 将flag改为2表示加入该群,0表示处于请求阶段,并且给这个人发消息
    if(!strcmp(msg.mg,"Y")){
        printf("同意[%d]的入群[%d]请求\n",msg.receiver,msg.sender);
        Set_flag_persist(msg.receiver,msg.sender);
        //发送消息通知加群成功flag= 20
        sprintf(s,"您已成功加入群[%d]",msg.sender);
        strcpy(msgfs.mg,s);
        memcpy(buf2,&msgfs,sizeof(msgfs));
        Receive_message2(fd,buf2);
    }else{
        printf("拒绝[%d]的入群[%d]请求\n",msg.receiver,msg.sender);
         //发送消息拒绝加群flag= 20
        sprintf(s,"您被拒绝加入群[%d]",msg.sender);
        strcpy(msgfs.mg,s);
        memcpy(buf2,&msgfs,sizeof(msgfs));
        Receive_message2(fd,buf2);
    }

}

int Get_grpmem_persist(char *buf)
{
    int fd;
    chat_message msg;
    chat_message msgfs;
    char buf2[MAXSIZE];
    memcpy(&msg,buf,sizeof(msg));
    
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  pertogrp where (id =  %d)",msg.receiver );
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;
    
    //msg.sender,msg.receiver,msg.mg);//发送者 群号 和聊天记录

    //初始化要发送的消息
    msgfs.flag = 20;
    msgfs.sender = msg.receiver;
    //消息需要处理一下
    char s[260];
    sprintf(s,"[%d]:%s",msg.sender,msg.mg);
    strcpy(msgfs.mg,s);

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){

        //将群消息转化成一对多的群通知 
        if(atoi(row[0]) != msg.sender)//群消息不要发给自己
        {
            msgfs.receiver = atoi(row[0]);
            memcpy(buf2,&msgfs,sizeof(msg));
            Receive_message2(fd,buf2); //发送的消息 sender 群号 receiver 接受者 msg 群消息
        }
        

    }
}

 void Store_grpmsg_ppersist(int sender,int id,char *mg)
 {//将群聊天记录写入数据库中
    int i,j;
    int flag = 0;
    char insert[MAXSIZE];
    char s[260];
    
    int len = strlen(mg);
    for(i = 0;i<len;i++){
        if(mg[i] == '\''){
            flag = 1;
            printf("----- --- %c\n",mg[i]);
            break;
        }
    }
    if(flag)
    {
         s[len+1] = '\0';
        for(j = len;j>i;j--){
             mg[j] = mg[j-1];
       }
    }

    sprintf(s,"[%d]:%s",sender,mg);
    sprintf(insert,"insert into grpmsg (sender,id,msg) values('%d','%d','%s')",sender,id,s);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }else printf("群聊天记录成功写入数据库\n");

 }

 int If_member(char *buf)
 {
      int fd;
    chat_message msg;
    chat_message msgfs;
    char buf2[MAXSIZE];
    memcpy(&msg,buf,sizeof(msg));
    
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  pertogrp where (username = %d and id =  %d)",msg.sender,msg.receiver );
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);
    
    //msg.sender,msg.receiver,msg.mg);//发送者 群号 和聊天记录



    //没有找到数据，说明不是群成员
    if(row == NULL ) return 0;
    else return 1;
 }

void Broadcast_grpmsg(int fd,char *buf)
{//sdender 发送者 receiver 群号 msg 消息
    
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    // printf("msg.sender = %d\n",msg.sender);
    // printf("msg.receiver = %d\n",msg.receiver);
    // printf("msg.mg = %s\n",msg.mg);

    //首先判断是不是群成员
    if(!If_member(buf)){
        Send_message(fd,0,"您还不是群成员");
        return ;
    }
    //将群聊天记录存入数据库中
    Store_grpmsg_ppersist(msg.sender,msg.receiver,msg.mg);//发送者 群号 和聊天记录

    //获取群成员并且广播给群成员
    Get_grpmem_persist(buf);
}

int Show_grp_persist(int fd,char *buf)
{
    group grp;
    int flag = 1;
    chat_message msg;
    int username;
    char str[260];
    memcpy(&msg,buf,sizeof(msg));
    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from  pertogrp where (username = %d )",msg.sender);
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){

        int id = atoi(row[1]);
        Get_grpbaseinfo_persist(grp,id);
        sprintf(str,"||%5d  %5d %5d %5d %5d||\n",grp.id,grp.owner,grp.mg1,grp.mg2,grp.mg3);
        Send_message(fd,22,str);

    }
    return flag;  
}

 void  Show_grp(int fd,char *buf)
 {
     chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    if(Show_grp_persist(fd,buf)){
        Send_message(fd,22,"over");
        printf("[%d]查询群成功\n",msg.sender);
    }else printf("[%d]查询群失败\n",msg.sender);
    return ;
 }

//从数据库中群成员关系
int Delete_grp_persist(int fd,char *buf)
{
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char update[256];
    sprintf(update,"delete from  pertogrp where (username  = %d and id = %d )",msg.sender,msg.receiver);
    if(mysql_real_query(con,update,strlen(update))){
        finish_with_error(con);
    }else {
        char s[256];
        printf("[%d]退出群[%d]\n",msg.sender,msg.receiver);
        sprintf(s,"[%d]退出群[%d]",msg.sender,msg.receiver);
        //通知群主
        group grp;
         Get_grpbaseinfo_persist(grp,msg.receiver);
            Send_message(getfd(grp.owner),0,s);

        //如果退群者是群主，群直接解散
        if(msg.sender == grp.owner){
            char s2[256];
             sprintf(s2,"delete from  pertogrp where (id = %d )",msg.receiver);
              if(mysql_real_query(con,update,strlen(update))){
        finish_with_error(con);
            }else {
                 Send_message(getfd(grp.owner),0,"群已经解散");
            }
        }
        //Send_message(getfd(msg.sender),0,"您已经成功退群\n");
        return 1;
    }
    return 0;
}

void Delete_grp(int fd,char *buf)
{
    //printf("yyyyy\n");
    chat_message msg;
    memcpy(&msg,buf,sizeof(msg));
    char s1[50];
    if(Delete_grp_persist(fd,buf)){
        sprintf(s1,"您已经退出群[%d]\n",msg.receiver);
        Send_srmessage(20,msg.sender,msg.sender,s1);//10
    }
    else printf("退群失败\n");
    return ;
}

 void Query_grpmsg(int fd,char *buf)
 {
     chat_message msg;
    memcpy(&msg,buf,sizeof(msg));

    if(Query_group_persist(fd,buf)){
        
        printf("[%d]查询群聊天记录成功\n",msg.sender);
    }else printf("[%d]查询群聊天记录失败\n",msg.sender);

    Send_message(fd,26,"oover");
    return ;
 }

 //cmessgae.msg为空  查询对象保存在receiver中
int Query_group_persist(int fd,char *buf)
{
    int flag = 1;
    chat_message msg;
    char str[50];
    memcpy(&msg,buf,sizeof(msg));


    //先判断是不是群成员
    char insert[MAXSIZE];
    sprintf(insert,"select  *from pertogrp  where (username = %d and  id = %d)",msg.sender,msg.receiver);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }
    MYSQL_RES *res2  = mysql_store_result(con);
    MYSQL_ROW  row2 = mysql_fetch_row(res2);

    if(row2 == NULL){
        Send_message(fd,0,"你还不是群成员");
        return 0;
    }


    char insert2[MAXSIZE];
    sprintf(insert2,"select  *from grpmsg   where ( id = %d)",msg.receiver);
    if(mysql_real_query(con,insert2,strlen(insert2))){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row ;

    //循环读取每一行数据
    while(row = mysql_fetch_row(res)){
        Send_message(fd,26,row[2]);//传过去后flag = 16
        }
        
    return flag;
}