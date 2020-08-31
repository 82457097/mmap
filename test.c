#include<stdio.h>
#include<sys/mman.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/stat.h>
#include<unistd.h>

int main(int argc, char *argv[]) {
	int fd = 0;
	char *ptr = NULL;
	struct stat buf = {0};
	
	if(argc < 2) {
		printf("please enter a file.\n");
		return -1;
	}

	if((fd = open(argv[1], O_RDWR)) < 0) {
		printf("open file error.\n");
		return -1;
	}

	if(fstat(fd, &buf) < 0) {
		printf("get file state error:%d\n", errno);
		close(fd);
		return -1;
	}

	ptr = (char *)mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		printf("mmap failed\n");
		close(fd);
		return -1;
	}
	close(fd);

	printf("length of the file is : %d\n", buf.st_size);
	printf("the %s content is : %s\n", argv[1], ptr);
	
	ptr[3] = 'a';
	printf("the %s new content is : %s\n", argv[1], ptr);
	munmap(ptr, buf.st_size);
	return 0;
}
	
