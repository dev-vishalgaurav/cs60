#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(){
	int pid = -1;
	pid = fork();
	if(pid == 0){
		printf("Child\n");
		sleep(1);
		printf("Child\n");
	}else{
		sleep(1);
		printf("parent %d\n",pid);
	}

	return 0;
}