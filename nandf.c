#include <stdio.h>
#include <stdlib.h>

#include <time.h>

// brute force nand programs up to...
#define ARGC 4
#define LENGTH 12

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
    valr[x] = ~(vals[a] & vals[b]);

#define EVAL(size, core) \
  int __eval##size(struct nand in, int *vals, int start) { \
    int a, b; \
    int *valr = vals + ARGC; \
    switch (start) { \
      core \
    } \
    return valr[size - 1]; \
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
#define STAGE9 STAGE8 STAGE(8)
EVAL(9, STAGE9)
#define STAGE10 STAGE9 STAGE(9)
EVAL(10, STAGE10)
#define STAGE11 STAGE10 STAGE(10)
EVAL(11, STAGE11)
#define STAGE12 STAGE11 STAGE(11)
EVAL(12, STAGE12)

int (*evals[13])(struct nand, int*, int) = {
  NULL,
  __eval1,
  __eval2,
  __eval3,
  __eval4,
  __eval5,
  __eval6,
  __eval7,
  __eval8,
  __eval9,
  __eval10,
  __eval11,
  __eval12
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
    if (__builtin_expect(prev->prog[i].b < prev->argc + i, 1)) {
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
  // we are checking all four-bit inputs

  // initialize test prog
  struct nand goal = (struct nand) { .argc = ARGC, .size = 1, .prog = NULL };
  goal.prog = (struct inst*) malloc(sizeof(struct inst) * 1);
  goal.prog[0] = (struct inst) { .a = 0, .b = 0 };

  // calculate total number of 4-arg programs up to size 16
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
  for (int i = 0; i < 16; i++) {
    int x = (i & 0x1);
    int y = (i & 0x2) >> 1;
    int z = (i & 0x4) >> 2;
    int w = (i & 0x8) >> 3;

    vals[0] |= x << i;
    vals[1] |= y << i;
    vals[2] |= z << i;
    vals[3] |= w << i;

    correct |= (x ^ y ^ z ^ w) << i;
  }

  int valid = 0; // the context is valid up to which inst?

  // main program check loop
  unsigned long long checked = 0;
  int (*cur_eval)(struct nand, int*, int) = evals[goal.size];
  while (goal.size != LENGTH + 1) {
    // check bitpacked outputs
    if (__builtin_expect(correct == cur_eval(goal, vals, valid), 0)) {
      print_nand(&goal);
      break;
    }

    // advance to next program
    valid = next(&goal);

    // realloc when prog size increased
    if (__builtin_expect(valid == -1, 0)) {
      valid = 0;
      vals = realloc(vals, sizeof(int) * (goal.argc + goal.size));
      cur_eval = evals[goal.size];
    }

    checked++;
    if (__builtin_expect(checked % 100000000 == 0, 0)) {
      printf("checked: %llu of < %llu\n", checked, total);
      printf("rate:    %llu\n", checked / clock());
    }
  }

  printf("checked: %llu of < %llu\n", checked, total);
  printf("in time: %ld\n", clock());
  printf("check/ms: %llu\n", checked / clock());
}
