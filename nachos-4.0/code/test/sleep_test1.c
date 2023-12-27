#include "syscall.h"
int main(){
    int i;
    for(i=0;i<20;++i){
        Sleep(10000);
        PrintInt(1);
    }
}