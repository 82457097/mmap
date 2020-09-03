#include<stdio.h>
#include<sys/mman.h>
#include<unistd.h>
#include<sys/types.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<string.h>

void mmapcopy(int sfd, size_t slen, int dfd) {
	void *sptr = mmap(NULL, slen, PROT_READ, MAP_SHARED, sfd, 0);
	void *dptr = mmap(NULL, slen, PROT_READ | PROT_WRITE, MAP_SHARED, dfd, 0);
	if(dptr == MAP_FAILED) {
		printf("mmap failed.");
		return;
	}
	
	memcpy(dptr, sptr, slen);

	munmap(sptr, slen);
	munmap(dptr, slen);
}

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

	struct stat fst;
	fstat(sfd, &fst);
	truncate(argv[2], fst.st_size);
	mmapcopy(sfd, fst.st_size, dfd);

	close(sfd);
	close(dfd);

	return 0;
}
