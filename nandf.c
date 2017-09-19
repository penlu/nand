#include <stdio.h>
#include <stdlib.h>

#include <time.h>

// brute force nand programs up to...
#define ARGC 3
#define LENGTH 8

struct inst {
  int a;
  int b;
};

struct nand {
  int           argc; // number of arguments
  int           size; // insts in program
  struct inst*  prog; // program
};

// partial eval from given program index
// macro on the size of the program
#define STAGE(x) \
  case x: \
    a = in.prog[x].a; \
    b = in.prog[x].b; \
    vals[ARGC + x] = !(vals[a] && vals[b]);

#define EVAL(size, core) \
  int __eval##size(struct nand in, int *vals, int start) { \
    int a, b; \
    switch (start) { \
      core \
    } \
    return vals[ARGC + size - 1]; \
  }

#define STAGE1 STAGE(0)
EVAL(1, STAGE1)
#define STAGE2 STAGE1 STAGE(1)
EVAL(2, STAGE2)
#define STAGE3 STAGE2 STAGE(2)
EVAL(3, STAGE3)
#define STAGE4 STAGE3 STAGE(3)
EVAL(4, STAGE4)
#define STAGE5 STAGE4 STAGE(4)
EVAL(5, STAGE5)
#define STAGE6 STAGE5 STAGE(5)
EVAL(6, STAGE6)
#define STAGE7 STAGE6 STAGE(6)
EVAL(7, STAGE7)
#define STAGE8 STAGE7 STAGE(7)
EVAL(8, STAGE8)

int (*evals[9])(struct nand in, int *vals, int start) = {
  NULL,
  __eval1,
  __eval2,
  __eval3,
  __eval4,
  __eval5,
  __eval6,
  __eval7,
  __eval8
};

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

  // signal prog size increased
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

  // initialize test prog
  struct nand goal = (struct nand) { .argc = ARGC, .size = 1, .prog = NULL };
  goal.prog = (struct inst*) malloc(sizeof(struct inst) * 1);
  goal.prog[0] = (struct inst) { .a = 0, .b = 0 };

  // calculate total number of 3-arg programs up to size 8
  unsigned long long total = 0;
  unsigned long long accum = 1;
  for (int i = 0; i < LENGTH; i++) {
    accum *= (i + goal.argc) * (i + goal.argc);
    total += accum;
  }

  // we are checking all three-bit inputs
  int *ctxs[8]; // maintain 8 partial evaluation contexts for program
  int valid[8]; // this context is valid up to which inst?
  for (int i = 0; i < 8; i++) {
    ctxs[i] = malloc(sizeof(int) * (goal.argc + goal.size));

    // input init
    ctxs[i][0] = (i & 0x1);
    ctxs[i][1] = (i & 0x2) >> 1;
    ctxs[i][2] = (i & 0x4) >> 2;

    valid[i] = 0;
  }

  // main program check loop
  unsigned long long checked = 0;
  int next_valid;
  while (goal.size != LENGTH + 1) {
    for (int i = 0; i < 8; i++) {
      int x = (i & 0x1);
      int y = (i & 0x2) >> 1;
      int z = (i & 0x4) >> 2;

      int from = valid[i];
      valid[i] = goal.size;
      if ((x ^ y ^ z) != evals[goal.size](goal, ctxs[i], from)) {
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
    if (checked % 100000000 == 0) {
      printf("checked: %llu of < %llu\n", checked, total);
      printf("rate:    %llu\n", checked / clock());
    }
  }

  printf("checked: %llu of < %llu\n", checked, total);
  printf("in time: %ld\n", clock());
  printf("check/ms: %llu\n", checked / clock());
}
