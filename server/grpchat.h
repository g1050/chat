#ifndef GRPCHAT_
#define GRPCHAT_
#include "common.h"

typedef struct group
{
    int flag;
    int id;//群号id
    char name[30];
    int owner;
    int mg1;
    int mg2;
    int mg3;
}gruop;

void Creat_grp(int fd,char *buf);

int Creat_grp_persisit(group grp);

void Add_grp(int fd,char *buf);

int Get_grpbaseinfo_persist(group &grp,int id);

void Add_group_receipt(int fd,char *buf);

void Broadcast_grpmsg(int fd,char *buf);

 void  Show_grp(int fd,char *buf);

 int Show_grp_persist(int fd,char *buf);

 void Delete_grp(int fd,char *buf);

 int Delete_grp_persist(int fd,char *buf);

  void Store_grpmsg_ppersist(int sender,int id,char *mg);
  
   void Query_grpmsg(int fd,char *buf);

   int Query_group_persist(int fd,char *buf);
#endif