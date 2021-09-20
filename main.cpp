/*
  --------------------------------------
     ╭╩══╮╔══════╗╔══════╗╔══════╗ 
    ╭    ╠╣      ╠╣      ╠╣      ╣
    ╰⊙══⊙╯╚◎════◎╝╚◎════◎╝╚◎════◎╝
  --------------------------------------
  @date:   2021-九月-19
  @author: xiaomingZhang2020@outlook.com
  --------------------------------------
*/
#include <iostream>
#include "concurrency.h"
using namespace std;
int main(){
    cout<<"hello world"<<endl;
    Tsd tsd;
    tsd.set((int*)(-1));
    cout<<(int*)(tsd.get())<<endl;
    return 0;
}