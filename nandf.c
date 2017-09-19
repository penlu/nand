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
    vals[ARGC + x] = ~(vals[a] & vals[b]);

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
  // we are checking all three-bit inputs

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

  // we maintain a single partial evaluation context
  // data corresponding to different inputs is bitpacked:
  //                    data0   data1   data2   data3
  // input 0 - vals[0]  0       0       0       0
  // input 1 - vals[1]  0       0       1       1       ...
  // input 2 - vals[2]  0       1       0       1
  int *vals = malloc(sizeof(int) * (goal.argc + goal.size));
  for (int i = 0; i < goal.argc + goal.size; i++) {
    vals[i] = 0;
  }

  // we also bitpack the correct output
  int correct = 0;

  // start packin'
  for (int i = 0; i < 8; i++) {
    int x = (i & 0x1);
    int y = (i & 0x2) >> 1;
    int z = (i & 0x4) >> 2;

    vals[0] |= x << i;
    vals[1] |= y << i;
    vals[2] |= z << i;

    correct |= (x ^ y ^ z) << i;
  }

  int valid = 0; // the context is valid up to which inst?

  // main program check loop
  unsigned long long checked = 0;
  while (goal.size != LENGTH + 1) {
    if (correct == evals[goal.size](goal, vals, valid)) {
      print_nand(&goal);
      break;
    }

    valid = next(&goal);

    // realloc when prog size increased
    if (valid == -1) {
      valid = 0;
      vals = realloc(vals, sizeof(int) * (goal.argc + goal.size));
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
