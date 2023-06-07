#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h> 
#include <fcntl.h> 


int main (void) 
{
  const char* filename = "output";
	int fd;	
	fd = open (filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		perror ("Error openning file\n");
		exit (EXIT_FAILURE);
	}
  char* str = "b4 fork\n";
  write (fd, str, strlen(str));
  pid_t pid = fork();
  if (pid==-1) {
    perror ("Cannot fork\n");
    exit (EXIT_FAILURE);
  }
  else if (pid==0) {
		printf ("child\n");
		str = "Hello from the child process\n";
    write (fd, str, strlen(str));
    exit (EXIT_SUCCESS);
  }
  else {
		printf ("dad\n");
    int status;
    waitpid (pid, &status, 0);
		str = "Hello from daddy\n";
    write (fd, str, strlen(str));
    exit (EXIT_SUCCESS);
}

  return EXIT_SUCCESS;
}
