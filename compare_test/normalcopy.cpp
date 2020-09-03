#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<stdlib.h>
#include<fcntl.h>

int main(int argc, char* argv[]) {
	if(argc != 3) {
		printf("Usage: %s <src filename> <des filename>", argv[0]);
		return -1;
	}

	int sfd;
	int dfd;

	if((sfd = open(argv[1], O_RDONLY)) < 0) {
		printf("open src file failed.");
		return sfd;
	}

	if((dfd = open(argv[2], O_CREAT | O_RDWR, 0664)) < 0) {
		printf("create dst file failed.");
		return dfd;
	}

	char buf[1024];
	int ret;
	while(1) {
		ret = read(sfd, buf, 1024);
		if(ret < 0) {
			printf("read error.");
			break;
		}
		else if(ret == 0) {
			printf("read end.");
			break;
		}
		
		if(ret != write(dfd, buf, ret)) {
			printf("write error.");
			break;
		}

	}

	close(sfd);
	close(dfd);

	return 0;
}
