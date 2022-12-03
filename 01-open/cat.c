#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

char buffer[BUFFER_SIZE];

int main(int argc, char *argv[]) {
  // For cat, we have to iterate over all command-line arguments of
  // our process. Thereby, argv[0] is our program binary itself ("./cat").
  int idx;
  for (idx = 1; idx < argc; idx++) {
    int fd = open(argv[idx], O_RDONLY);
    if (fd < 0) {
      perror("open");
      return -1;
    }

    while (1) {
      ssize_t n = read(fd, buffer, BUFFER_SIZE);
      if (n == -1) {
        perror("read");
        return -1;
      } else if (n == 0) {
        break;
      }

      char *p = buffer;
      while (n > 0) {
        ssize_t written = write(1, p, n);
        if (written == -1) {
          perror("write");
          return -1;
        }
        p += written;
        n -= written;
      }
    }
    close(fd);
  }

  return 0;
}
