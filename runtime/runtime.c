#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_int(int x) { printf("%d", x); }
void print_char(int c) { putchar(c); }
void print_bool(int x) { printf("%d", x); }
void println() { putchar('\n'); }
void print_int_ln(int x) { printf("%d\n", x); }
int read_int() {
  int x;
  scanf("%d", &x);
  return x;
}
int read_char() { return getchar(); }

void cc_sleep(int milliseconds) { usleep(milliseconds * 1000); }
void cc_exit(int code) { exit(code); }
