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
#define EVAL_STAGE(x) \
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

// macros define eval for each possible size of the program
#define EVAL_STAGE1 EVAL_STAGE(0)
#define EVAL_STAGE2 EVAL_STAGE1 EVAL_STAGE(1)
#define EVAL_STAGE3 EVAL_STAGE2 EVAL_STAGE(2)
#define EVAL_STAGE4 EVAL_STAGE3 EVAL_STAGE(3)
#define EVAL_STAGE5 EVAL_STAGE4 EVAL_STAGE(4)
#define EVAL_STAGE6 EVAL_STAGE5 EVAL_STAGE(5)
#define EVAL_STAGE7 EVAL_STAGE6 EVAL_STAGE(6)
#define EVAL_STAGE8 EVAL_STAGE7 EVAL_STAGE(7)
#define EVAL_STAGE9 EVAL_STAGE8 EVAL_STAGE(8)
#define EVAL_STAGE10 EVAL_STAGE9 EVAL_STAGE(9)
#define EVAL_STAGE11 EVAL_STAGE10 EVAL_STAGE(10)
#define EVAL_STAGE12 EVAL_STAGE11 EVAL_STAGE(11)
EVAL(1, EVAL_STAGE1)
EVAL(2, EVAL_STAGE2)
EVAL(3, EVAL_STAGE3)
EVAL(4, EVAL_STAGE4)
EVAL(5, EVAL_STAGE5)
EVAL(6, EVAL_STAGE6)
EVAL(7, EVAL_STAGE7)
EVAL(8, EVAL_STAGE8)
EVAL(9, EVAL_STAGE9)
EVAL(10, EVAL_STAGE10)
EVAL(11, EVAL_STAGE11)
EVAL(12, EVAL_STAGE12)

// size-based eval function table
int (*evals[13])(struct nand, int*, int) = {
  NULL,     __eval1,  __eval2,  __eval3,
  __eval4,  __eval5,  __eval6,  __eval7,
  __eval8,  __eval9,  __eval10, __eval11,
  __eval12
};


// lookup table for next instruction
struct inst next_inst[(ARGC + LENGTH - 1) * (ARGC + LENGTH - 1) / 2 + \
                      (ARGC + LENGTH) / 2];
int max_inst[LENGTH];

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

  // calculate total number of 4-arg programs up to size 16
  unsigned long long total = 0;
  unsigned long long accum = 1;
  for (int i = 0; i < LENGTH; i++) {
    accum *= (i + ARGC) * (i + ARGC);
    total += accum;
  }

  // initialize test prog
  struct nand goal = (struct nand) { .argc = ARGC, .size = 1, .prog = NULL };
  goal.prog = malloc(sizeof(struct inst) * goal.size);
  goal.prog[0] = (struct inst) { .a = 0, .b = 0 };


  // initialize lookup tables for next inst
  int a = 0;
  for (int i = 0; i < ARGC + LENGTH - 1; i++) {
    for (int j = 0; j <= i; j++) {
      next_inst[a++] = (struct inst) { .a = i, .b = j };
    }
  }
  for (int i = 0; i < LENGTH; i++) {
    max_inst[i] = (i + ARGC) * (i + ARGC) / 2 + (i + ARGC + 1)/2;
  }

  // initialize next inst lookup vector
  int *pinst = malloc(sizeof(int) * (goal.size + 1));
  pinst[0] = 0;
  pinst[1] = 0;


  // we maintain a single partial evaluation context
  // data corresponding to different inputs is bitpacked:
  //                    data0   data1   data2   data3
  // input 0 - vals[0]  0       0       0       0
  // input 1 - vals[1]  0       0       1       1       ...
  // input 2 - vals[2]  0       1       0       1
  int *vals = malloc(sizeof(int) * (goal.argc + goal.size));
  for (int i = 0; i < goal.argc; i++) {
    vals[i] = 0;
  }

  // we also bitpack the correct output
  int correct = 0;

  // start packin'
  for (int i = 0; i < 8; i++) {
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

    // compute next nand program
    for (valid = goal.size - 1; valid != -1; valid--) {
      if (__builtin_expect(pinst[valid + 1]++ == max_inst[valid], 0)) {
        pinst[valid + 1] = pinst[valid];
        goal.prog[valid] = next_inst[pinst[valid + 1]];
      } else {
        goal.prog[valid] = next_inst[pinst[valid + 1]];
        break;
      }
    }

    // we have to increase the size
    if (__builtin_expect(valid == -1, 0)) {
      goal.size++;

      goal.prog = realloc(goal.prog, sizeof(struct inst) * goal.size);
      goal.prog[goal.size - 1] = (struct inst) { .a = 0, .b = 0 };
      pinst = realloc(pinst, sizeof(int) * (goal.size + 1));
      pinst[goal.size] = 0;

      vals = realloc(vals, sizeof(int) * (goal.argc + goal.size));
      cur_eval = evals[goal.size];

      valid = 0;
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
