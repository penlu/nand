#include <stdio.h>
#include <stdlib.h>

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

int eval(struct nand *in, int *argv) {
  // create value array
  int *vals = (int*) malloc(sizeof(int) * (in->argc + in->size));

  // init args
  for (int i = 0; i < in->argc; i++) {
    vals[i] = argv[i];
  }

  // evaluate
  for (int i = 0; i < in->size; i++) {
    int a = in->prog[i].a;
    int b = in->prog[i].b;
    vals[in->argc + i] = !(vals[a] && vals[b]);
  }

  // clean up value array
  int res = vals[in->argc + in->size - 1];
  free(vals);

  return res;
}

// compute next nand program
// argc fixed
void next(struct nand *prev) {
  for (int i = 0; i < prev->size; i++) {
    // inc inst arg 1
    prev->prog[i].a++;
    if (prev -> prog[i].a < prev->argc + i) {
      return;
    }

    // inc inst arg 2
    prev->prog[i].a = 0;
    prev->prog[i].b++;
    if (prev->prog[i].b < prev->argc + i) {
      return;
    }

    prev->prog[i].b = 0;
  }

  // we have to increase the size
  prev->size++;
  prev->prog = (struct inst*) realloc(prev->prog, sizeof(struct inst) * prev->size);
  prev->prog[prev->size - 1] = (struct inst) { .a = 0, .b = 0 };
}

void print_nand(struct nand *p) {
  printf("argc: %d\n", p->argc);
  printf("size: %d\n", p->size);
  for (int i = 0; i < p->size; i++) {
    printf("var %d: %d nand %d\n", i + p->argc, p->prog[i].a, p->prog[i].b);
  }
}

int main(int argc, char *argv[]) {
  // it's wired to brute force a xor
  
  struct nand goal = (struct nand) { .argc = 2, .size = 1, .prog = NULL };
  goal.prog = (struct inst*) malloc(sizeof(struct inst) * 1);
  goal.prog[0] = (struct inst) { .a = 0, .b = 0 };

  int i = 0;
  while (goal.size != 5) {
    int in[2] = {0, 0};
    int r1 = eval(&goal, in);
    in[1] = 1;
    int r2 = eval(&goal, in);
    in[0] = 1;
    int r3 = eval(&goal, in);
    in[1] = 0;
    int r4 = eval(&goal, in);

    if (!r1 && r2 && !r3 && r4) {
      print_nand(&goal);
    }

    next(&goal);

    i++;
    if (i % 1000000 == 0) {
      printf("%d\n", i);
    }
  }

  printf("%d\n", i);
}
