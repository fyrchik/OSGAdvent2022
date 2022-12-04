#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// We define two CPP macros to hide the ugliness of these compiler
// attributes.
//
// - section: This variable attribute pushes the variable into the
//            named ELF section. Thereby, we let the linker collect
//            variable definitions and concentrate them in one place
//
// - aligned: Normally, variables can be densely packed in the
//            data/BSS section. However, as we want to replace parts
//            of our data section with a file mapping, we obey the
//            MMU's granularity (4096 bytes).
#define persistent __attribute__((section("persistent")))
#define PAGE_SIZE 4096
#define page_aligned __attribute__((aligned(PAGE_SIZE)))

// This is anonymous struct describes our persistent data and a
// variable with that type, named psec, is defined.
struct /* anonymous */ {
    // We mark the first variable in our struct as being page aligned.
    // This is equivalent to a start address an address ending in 3
    // hexadecimal zeros.
    //
    // By aligning the first variable in the struct, two things happen:
    //
    // 1. Global variables of struct persistent_section are placed at
    //    the beginning of a page.
    // 2. The size of the section is padded to the next multiple of
    //    PAGE_SIZE, such that array-address calculations and pointer
    //    arithmetik work correctly.
    page_aligned int persistent_start;

   // This is our persistent variable. We will use it as a counter,
   // how often the program was executed.
    int foobar;
} psec;

// For comparision, we also create another global variable, that is
// initialized from the program ELF on every program start.
int barfoo = 42;

int setup_persistent(char *fn) {
  int fd = open(fn, O_RDWR | O_CREAT, 0660);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  if (ftruncate(fd, sizeof(psec)) == -1) {
    perror("ftruncate");
    goto cleanup;
  }

  void *addr = mmap(&psec, sizeof(psec), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_FIXED, fd, 0);
  if (addr == NULL) {
    perror("mmap");
    goto cleanup;
  }

  return 0;

cleanup:
  close(fd);
  return -1;
}

int main(int argc, char *argv[]) {
  printf("psec: %p--%p\n", &psec, &psec + 1);
  // Install the persistent mapping
  if (setup_persistent("mmap.persistent") == -1) {
    perror("setup_persistent");
    return -1;
  }

  // For psec.foobar, we see that each invokation of the programm will
  // yield an incremented result.
  // For barfoo, which is *NOT* in the persistent section, we will
  // always get the same result.
  printf("foobar(%p) = %d\n", &psec.foobar, psec.foobar++);
  printf("barfoo(%p) = %d\n", &barfoo, barfoo++);

  { // This is ugly and you should not do this in production code.

    // In order to see the memory mappings of the currently
    // running process, we use the pmap (for process-map) tool to
    // query the kernel (/proc/self/maps)
    char cmd[256];
    snprintf(cmd, 256, "pmap %d", getpid());
    printf("---- system(\"%s\"):\n", cmd);
    system(cmd);
  }

  return 0;
}
