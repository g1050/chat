#ifndef FILECHAT_
#define FILECHAT_

typedef struct file
{
    int flag;
    int sender;
    int receiver;
    long long size;
    long long pos;
    char name[100];
    char data[800];
}file;

void *Recv_file(void *buf);
#endif