#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
int i=0;
void f() {
 i++;
 printf("IN: i=%d\n",i);
 int pid= fork();
if (pid!=0) {
wait(NULL);
printf(“EXIT: i=%d\n”,i);
exit(0);
}
}
int main() {
 i=0;
 while (i<2)
 f();
 printf("END: i=%d\n",i);
exit(0);
}