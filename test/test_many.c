#include<stdio.h>
#include<stdlib.h>

int main(int argc, char* argv[])
{
  long unsigned int num = atoi(argv[1]);
  int i;
  char fname[20];
  char* buf = (char*)malloc(10);
  memset(buf, 'a', 10);
  for(i=0; i<num; i++) {
     //char* fname = itoa(i);
	sprintf(fname, "%d", i);
     FILE* fp = fopen(fname, "wt+");

     if (!fp) {
      printf("Error when creating file %s\n", fname);
      return -1;
     }
     long unsigned int written_sz = fwrite(buf, 1, 10, fp);
     fclose(fp);
   }
  free(buf);
  return 0;
  }
