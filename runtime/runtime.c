#include <stdio.h>
#include <stdlib.h>

void print_int(int x) { printf("%d", x); }
void print_char(char c) { putchar(c); }
void print_bool(int x) { printf("%d", x); }
void println() { putchar('\n'); }
void print_int_ln(int x) { printf("%d\n", x); }
int read_int() {
  int x;
  scanf("%d", &x);
  return x;
}
int read_char() { return getchar(); }

#ifdef _WIN32
#include <windows.h>
void cc_sleep(int ms) { Sleep(ms); }
#else
#include <unistd.h>
void cc_sleep(int ms) { usleep(ms * 1000); }
#endif
void cc_exit(int code) { exit(code); }
