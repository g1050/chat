#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include  "./common.h"
#include "pvachat.h"

int Account_register_persist(account reg);

void Store_question_persist(int fd,security_question sq);

void Acount_register(int fd,char *buf);

int  Account_veri_persist(int fd,char *buf);

int Account_veri(int fd,char *buf);

int Question_veri_persist(int fd,char  *buf);




#endif