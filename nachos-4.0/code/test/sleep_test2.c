#include "syscall.h"
int main(){
    int i;
    for(i=0;i<5;++i){
        Sleep(40000);
        PrintInt(0);
    }
}