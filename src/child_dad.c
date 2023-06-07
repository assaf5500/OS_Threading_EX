#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (void) 
{
	printf ("b4 fork: pid = %d ", getpid());
	fflush (stdout); ////http://stackoverflow.com/questions/2530663/printf-anomaly-after-fork
	pid_t pid = fork();
  if (pid==-1) {
    perror ("Cannot fork\n");
    exit (EXIT_FAILURE);
  }
  else if (pid==0) {
    printf ("Hello from the child process. My pid is %d\n", getpid());
  }
  else {
    int status;
    printf ("Hello from daddy. My pid is %d\n", getpid());
    waitpid (pid, &status, 0);
 }

  return EXIT_SUCCESS;
}


