#include<stdio.h>
#include<stdlib.h>

int main(int argc, char* argv[])
{
  FILE* fp = fopen(argv[1], "wt+");
  long unsigned int num = atoi(argv[2]);
  if (!fp) {
      printf("Error when creating file rw2\n");
      return -1;
  }
  char* buf = (char*)malloc(num);
  memset(buf, 'a', num);
  long unsigned int written_sz = fwrite(buf, 1, num, fp);
  printf("file rw2 size: %ld\n", written_sz);
  fclose(fp);
  free(buf);
  return 0;
  }
