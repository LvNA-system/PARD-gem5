#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int i;
	int fd;
	uint32_t *map;  /* mmapped array of int's */

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	map = (uint32_t *)mmap(0, 32, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i <=8; ++i) {
		printf("0x%x\n", map[i]);
	}

	if (munmap(map, 32) == -1) {
		perror("Error un-mmapping the file");
	}
	close(fd);
	return 0;
}
