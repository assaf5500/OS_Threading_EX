#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 

int main (void) 
{
	int fd_new, fd_old; 
	fd_old = open ("output", O_CREAT | O_TRUNC | O_RDWR, 0666);
	if (fd_old < 0) {
		perror ("Error openning file\n");
		exit (EXIT_FAILURE);
	}
	close (1);
	fd_new = dup (fd_old);
	close (fd_old);
	printf ("This line didn't go to stdout\n");
	
}
