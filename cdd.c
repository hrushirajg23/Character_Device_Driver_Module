
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include<string.h>
#include<stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#define DRIVER_NAME "/dev/cq_device"
#define SET_SIZE_OF_QUEUE _IOW('a', 'a', int * )
#define PUSH_DATA _IOW('a', 'b', struct data * )
struct data {
    int length;
    char * data;
};

int main(void) {
    int fd = open(DRIVER_NAME, O_RDWR);
    if(fd==-1){
        printf("dereference issue\n");
    }
    int size = 100;
    int ret = ioctl(fd, SET_SIZE_OF_QUEUE, & size);

    //  if(fork()==0){

    //       fd = open(DRIVER_NAME, O_RDWR);
    //       struct data * d = malloc(sizeof(struct data));
    //       d->length = 3;
    //       d->data = malloc(3);
    //       memcpy(d->data, "xyz", 3);
    //       int ret = ioctl(fd, PUSH_DATA, d);
    //       close(fd);
    //       free(d->data);
    //       free(d);
    //       exit(0);
    //  }

    close(fd);
    if(ret==-EFAULT){
        perror("no access to page\n");
    }
    return ret;
}
