#include <stdio.h>
#include "client.h"
int main(){
  int ret;
  do{
      ret=authentification();
    }while(ret!=NO_PB);
    return 0;
}
