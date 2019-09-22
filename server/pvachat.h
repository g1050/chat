#ifndef PVACHAT_H_
#define PVACHAT_H_
#include "common.h"

int Add_friend_persist(int sender,int receiver);

int Add_friend(int fd,char *buf);

int Receive_message(int  fd,char *buf);

void Add_friend_receipt(int fd,char *buf);

int Add_friend_repisist(int fd,char *buf);

void Delete_friend(int fd,char *buf);

int Delete_friend_persist(int fd,char *buf);

void Shield_frinend(int fd,char *buf);

int  Shield_frinend_persist(int fd,char *buf);

 void Relieve_friend(int fd,char *buf);

 int Relieve_friend_persist(int fd,char *buf);

  void Show_friend(int fd,char *buf);

  int Show_friend_persist(int fd,char *buf);

  int Check_shield(int fd,int sender,int receiver);

  void Send_offmsg_persist2(int recvfd,int receiver);
  
  void Send_offreq_persist(int fd,int receiver);

void Query_message(int fd,char *buf);

int Query_friend_persist(int fd,char *buf);

int Receive_message2(int  fd,char *buf);

void Send_offmsg_persist2(int recvfd,int sender ,int receiver);
#endif