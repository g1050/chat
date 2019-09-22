#ifndef CLIENT_H_
#define CLIENT_H_

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

#define LISTENQ 12                    //连接请求队列的最大长度
#define SERV_ADDRESS  "192.168.3.191"
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
    int size;
    char name[100];
    char data[800];
}file;

#endif