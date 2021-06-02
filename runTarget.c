#include "stdio.h"

extern float target();

int main() {
  float val = target();
  printf("Return value from target(): %f\n", val);
}
