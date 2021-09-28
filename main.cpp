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
#include "include/gcmalloc.h"
using namespace std;

int main(){
    gcmalloc gc;
    auto p = gc.malloc(10);
    auto p1 = gc.malloc(100);
    auto p2 = gc.malloc(1000);

    return 0;
}