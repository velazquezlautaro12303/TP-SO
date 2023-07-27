#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct 
{
	uint32_t BLOCK_SIZE;
	uint32_t BLOCK_COUNT;
} SUPER_BLOCK;

void main()
{
	int fd = open("superbloque.dat", O_RDWR);

	SUPER_BLOCK superBlock;
	superBlock.BLOCK_SIZE = 64;
	superBlock.BLOCK_COUNT = 65536;

	write(fd, &superBlock, sizeof(SUPER_BLOCK));

	close(fd);
}
