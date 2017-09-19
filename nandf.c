#include <stdio.h>
#include <stdlib.h>

#include <time.h>

// brute force nand programs

struct inst {
  int a;
  int b;
};

struct nand {
  int           argc; // number of arguments
  int           size; // insts in program
  struct inst*  prog; // program
};
// len(vals) = argc + size + 1 (the result)

// partial eval from given program index
int eval(struct nand in, int *vals, int start) {
  for (int i = start; i < in.size; i++) {
    int a = in.prog[i].a;
    int b = in.prog[i].b;
    vals[in.argc + i] = !(vals[a] && vals[b]);
  }

  return vals[in.argc + in.size - 1];
}

// compute next nand program
// argc is fixed
// return the lowest prog index changed
int next(struct nand *prev) {
  for (int i = prev->size - 1; i >= 0; i--) {
    // inc inst arg 1
    prev->prog[i].a++;
    if (prev -> prog[i].a < prev->argc + i) {
      return i;
    }

    // inc inst arg 2
    prev->prog[i].a = prev->prog[i].b + 1;
    prev->prog[i].b++;
    if (prev->prog[i].b < prev->argc + i) {
      return i;
    }

    prev->prog[i].a = 0;
    prev->prog[i].b = 0;
  }

  // we have to increase the size
  prev->size++;
  prev->prog = (struct inst*) realloc(prev->prog, sizeof(struct inst) * prev->size);
  prev->prog[prev->size - 1] = (struct inst) { .a = 0, .b = 0 };

  return -1;
}

void print_nand(struct nand *p) {
  printf("argc: %d\n", p->argc);
  printf("size: %d\n", p->size);
  for (int i = 0; i < p->size; i++) {
    printf("var %d: %d nand %d\n", i + p->argc, p->prog[i].b, p->prog[i].a);
  }
}

int main(int argc, char *argv[]) {
  // it's wired to brute force a xor

  struct nand goal = (struct nand) { .argc = 3, .size = 1, .prog = NULL };
  goal.prog = (struct inst*) malloc(sizeof(struct inst) * 1);
  goal.prog[0] = (struct inst) { .a = 0, .b = 0 };

  // checking all three-bit inputs
  int *ctxs[8]; // maintain 8 partial evaluation contexts for program
  int valid[8]; // context is valid up to which inst?
  for (int i = 0; i < 8; i++) {
    ctxs[i] = malloc(sizeof(int) * (goal.argc + goal.size));

    // input init
    ctxs[i][0] = (i & 0x1);
    ctxs[i][1] = (i & 0x2) >> 1;
    ctxs[i][2] = (i & 0x4) >> 2;

    valid[i] = 0;
  }

  unsigned long long checked = 0;
  int next_valid;
  while (goal.size != 10) {
    for (int i = 0; i < 8; i++) {
      int x = (i & 0x1);
      int y = (i & 0x2) >> 1;
      int z = (i & 0x4) >> 2;

      int upto = valid[i];
      valid[i] = goal.size;
      if ((x ^ y ^ z) != eval(goal, ctxs[i], upto)) {
        goto next_prog;
      }
    }

    print_nand(&goal);
    break;

next_prog:
    next_valid = next(&goal);

    // realloc when prog size increased
    if (next_valid == -1) {
      next_valid = 0;

      for (int i = 0; i < 8; i++) {
        ctxs[i] = realloc(ctxs[i], sizeof(int) * (goal.argc + goal.size));
      }
    }

    // invalidate contexts
    for (int i = 0; i < 8; i++) {
      if (valid[i] > next_valid) {
        valid[i] = next_valid;
      }
    }

    checked++;
    if (checked % 1000000 == 0) {
      printf("checked: %llu\n", checked);
    }
  }

  printf("checked: %llu\n", checked);
  printf("in time: %ld\n", clock());
  printf("check/ms: %llu\n", checked / clock());
}
