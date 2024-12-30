#include<stdio.h>
#include<stdlib.h>

struct cost{
   int init;
    float die; 
};
struct Bee{
     int len;
     char* space;
     struct cost* cost;
};


int main(){
     struct Bee *bee=(struct Bee*)malloc(sizeof(struct Bee));
     void* arg=bee;
     bee->len=5;
     bee->space=(char*)malloc((bee->len)*(sizeof(char)));
     bee->cost=(struct cost*)malloc(sizeof(struct cost));
     bee->cost->die=5.0f;
     bee->space="ring";
     void* ptr=((*(struct Bee*)arg).space);

     float val=(((struct Bee*)arg)->cost)->die;
     printf("Bee.len = %d\nBee.space= %s\n",bee->len,bee->space); 
     printf("ptr is also %s\n",(char*)ptr);
     printf("cost float val is %.1f\n",val);

     int* bruh=(int*)bee;

     printf("bruh is %d\n",*bruh);

     return 0;
}