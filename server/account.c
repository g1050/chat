#include "account.h"
#include "pvachat.h"

//返回注册的账号失败返回-1
int Account_register_persist(account reg)
{
    char insert[MAXSIZE];
    sprintf(insert,"insert into Account (passwd,nickname) values('%s','%s')",reg.passwd,reg.nickname);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }else printf("新账号成功写入数据库\n");

    //再从数据库中获取插入的主键值
    if(mysql_query(con,"select LAST_INSERT_ID()")){
        finish_with_error(con);
    }
    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);
    // printf("res = %d\n",atoi(row[0]));
    // printf("标志是%d\n",reg.flag);
    // printf("昵称是%s\n",reg.nickname);
    // printf("密码是%s\n",reg.passwd);
    return atoi(row[0]);
}
void Store_question_persist(int fd,security_question sq)
{
    char insert[MAXSIZE];
    sprintf(insert,"insert into security_question values('%d','%s','%s')",sq.username,sq.phonenum,sq.home);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }else printf("新密保成功写入数据库\n");
}

void Store_question(int fd,security_question sq)
{
    printf("-------------------\n");
    Store_question_persist(fd,sq);
    printf("username = %d\n",sq.username);
    printf("phone = %s\n",sq.phonenum);
    printf("home = %s\n",sq.home);
    return ;
}

void Acount_register(int fd,char *buf)
{
    union_account ua;
    memcpy(&ua,buf,sizeof(ua));
    account reg;
    security_question sq;
    strcpy(sq.home,ua.home);
    strcpy(sq.phonenum,ua.phonenum);
    strcpy(reg.passwd,ua.passwd);
    strcpy(reg.nickname,ua.nickname);
    char str[MAXSIZE];
    int ret = Account_register_persist(reg);
    if(ret < 0){
        my_err("注册账号失败",__LINE__);
    }
    else{
        sq.username = ret;
        Store_question(fd,sq);
        char str2[256];
        sprintf(str2,"新账号[%d]注册成功",ret);
        printf("str2\n");
        Send_message(fd,0,str2);
    }
    
    
}

int  Account_veri_persist(int fd,char *buf)
{
    account reg;
    memcpy(&reg,buf,sizeof(reg));

    //首先判断是否已经登录
    if(getfd(reg.username) > 0){//小于表示套接字不存在
        printf(">- 该账号已登录 -<\n");
        return 2;
    }
    char insert[MAXSIZE];
    sprintf(insert,"select  *from  Account where username = %d",reg.username);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }
    //再从数据库中获取插入的主键值

    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);
    if(row == NULL) {
             return -1;
    }
    else {
             if(!strcmp(row[1],reg.passwd)) {
                add_node(fd,reg.username);
                onlion_list_t p;
                List_ForEach(list,p){
                    printf("username -> %d\n",p->data.username);
                    printf("fd->%d\n",p->data.fd);
                }
                return 1;
             }
              else return -1;
    }
   
}
int Account_veri(int fd,char *buf)
{
    char str[MAXSIZE];
    account reg;
    memcpy(&reg,buf,sizeof(reg));
    int ret = Account_veri_persist(fd,buf);
    if(ret == -1 ){//密码错误
        Send_message(fd,2,"N");
    }
    else if(ret == 2) Send_message(fd,2,"R");//重复登录
    else{//登录成功加载离线消息
        Send_message(fd,2,"Y");
        Send_offmsg_persist2(fd,reg.username);//离线聊天信息
        Send_offreq_persist(fd,reg.username);///离线好友请求
    } 

}


int Question_veri_persist(int fd,char  *buf)
{
    puts("----------------\n");
    security_question sq;
    memcpy(&sq,buf,sizeof(sq));

    printf("%s\n",sq.newpasswd);

    char insert[MAXSIZE];
    sprintf(insert,"select  *from  security_question where username = %d",sq.username);
    if(mysql_real_query(con,insert,strlen(insert))){
        finish_with_error(con);
    }

    MYSQL_RES *res  = mysql_store_result(con);
    MYSQL_ROW  row = mysql_fetch_row(res);

    if(row == NULL) {
             Send_message(fd,0,"密保答案错误或账号不存在");
    }
    else {
        if(!strcmp(row[1],sq.phonenum) && !strcmp(row[2],sq.home)){
            char update[256];
            sprintf(update,"update Account set passwd = '%s' where username = '%d'",sq.newpasswd,sq.username);
            if(mysql_real_query(con,update,strlen(update))){
                finish_with_error(con);
            }else {
                Send_message(fd,0,"新密码修改成功");
            }
        }
      else Send_message(fd,0,"密保答案错误或账号不存在");
    }
}

// int Question_veri(int fd,char *buf)
// {
//     char str[MAXSIZE];
//     int ret = Question_veri_persist(buf);
//     if(ret < 0){
//         Send_message(fd,4,"N");
//     }
//     else {
//         Send_message(fd,4,"Y");
//     }
// } 

// int Passwd_mod_persist(int fd,char *buf)
// {
//     char update[256];
//     account ac;
//     memcpy(&ac,buf,sizeof(ac));
//     printf("username = %d\n",ac.username);
//     printf("passwd = %d\n",ac.passwd);
//     sprintf(update,"update Account set passwd = %s where username = %d",ac.passwd,ac.username);
//     if(mysql_real_query(con,update,strlen(update))){
//         finish_with_error(con);
//     }else {
//         Send_message(fd,0,"新密码修改成功");
//     }
// }


    
    // if(Account_exist())
    //     if(send(fd,"N",MAXSIZE,0)< 0) my_err("send err",__LINE__);

