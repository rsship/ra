#include <stdio.h>
#include <stdbool.h>
#define STB_AGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#define WIDTH 800
#define HEIGHT 800

static Arena node_arena = {0};

typedef struct {
  uint8_t r; 
  uint8_t g; 
  uint8_t b; 
  uint8_t a; 
} RGBA32;

static RGBA32 pixels[WIDTH*HEIGHT];

typedef struct Node Node;

typedef enum {
  NK_X,
  NK_Y,
  NK_NUMBER,
  NK_BOOL,
  NK_ADD,
  NK_MULT,
  NK_COLOR,
  NK_COND,
  NK_GT,
  NK_GTE,
  NK_MOD,
  RULE_OR
} Node_Kind;

typedef struct {
  Node *lhs;
  Node *rhs;
} Node_Ops;

typedef struct {
  Node *statement;
  Node *iff;
  Node *elsee;
} Node_Cond;

typedef struct {
  Node *r;
  Node *g;
  Node *b;
} Node_Color;

typedef union {
  Node_Kind kind;
  Node_Ops  ops;
  float number;
  bool boolean;
  Node_Color color;
  Node_Cond cond;
} Node_As;

struct Node {
  Node_Kind kind;
  Node_As as;
};

typedef struct {
  Node *node;
} Rule;

typedef struct {
  Rule **rule;
  size_t capacity;
  size_t count;
  float probability;
} Rule_Or;

Node *node_x()
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_X;
  return node;
}

Node *node_y()
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_Y;
  return node;
}

Node *node_num(float num)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_NUMBER;
  node->as.number = num;
  return node;
}

Node *node_boolean(bool boolean)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_BOOL;
  node->as.boolean = boolean;
  return node;
}

Node *node_color(Node *r, Node *g, Node *b)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_COLOR;
  node->as.color.r = r;
  node->as.color.g = g;
  node->as.color.b = b;
  return node;
}

Node *node_gt(Node *lhs, Node *rhs)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_GT;
  node->as.ops.lhs = lhs;
  node->as.ops.rhs = rhs;
  return node;
}

Node *node_gte(Node *lhs, Node *rhs)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_GTE;
  node->as.ops.rhs = rhs;
  node->as.ops.lhs = lhs;
  return node;
}

Node *node_mult(Node *lhs, Node *rhs)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_MULT;
  node->as.ops.lhs = lhs;
  node->as.ops.rhs = rhs;
  return node;
}

Node *node_mod(Node *lhs, Node *rhs)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_MOD;
  node->as.ops.lhs = lhs;
  node->as.ops.rhs = rhs;
  return node;
}

Node *node_if(Node *statement, Node *iff, Node *elsee)
{
  Node *node = arena_alloc(&node_arena, sizeof(Node));
  node->kind = NK_COND;
  node->as.cond.statement = statement;
  node->as.cond.iff = iff;
  node->as.cond.elsee = elsee;
  return node;
}

bool expect_num(Node *x)
{
    if (!x) return false;
    if (x->kind != NK_NUMBER) {
      printf("expected number\n");
      return false;
    }
    return true;
}

bool expect_bool(Node *x)
{
    if (!x) return false;
    if (x->kind != NK_BOOL)  {
      printf("expected bool\n");
      return false;
    }
    return true;
}

bool expect_color(Node *x)
{
  if (!x) return false;
  if (x->kind != NK_COLOR) {
    return false;
  }
  return true;
}

Node *node_eval(Node *f, float x, float y)
{
  switch (f->kind) {
  case NK_X:      return node_num(x);
  case NK_Y:      return node_num(y);
  case NK_NUMBER: return f;
  case NK_BOOL:   return f;
  case NK_ADD: {
    Node *lhs = node_eval(f->as.ops.lhs, x,  y);
    if (!expect_num(lhs)) return NULL;

    Node *rhs = node_eval(f->as.ops.rhs, x, y);
    if (!expect_num(rhs))  return NULL;

    return node_num(lhs->as.number + rhs->as.number);

    // problem is not in here;
  }
  case NK_MULT: {
    Node *lhs = node_eval(f->as.ops.lhs, x,  y);
    if (!expect_num(lhs)) return NULL;

    Node *rhs = node_eval(f->as.ops.rhs, x,  y);
    if (!expect_num(rhs)) return NULL;

    return node_num(lhs->as.number * rhs->as.number);
  }
  case NK_COLOR: {
    Node *red = node_eval(f->as.color.r, x, y);
    if (!red) return NULL;
    Node *green = node_eval(f->as.color.g, x, y);
    if (!green) return NULL;

    Node *blue = node_eval(f->as.color.b, x, y);
    if (!blue) return NULL;

    return node_color(red, green, blue);
  }
  case NK_GT: {
    Node *lhs = node_eval(f->as.ops.lhs, x, y);
    if (!expect_num(lhs)) return NULL;

    Node *rhs = node_eval(f->as.ops.rhs, x, y);
    if (!expect_num(rhs)) return NULL;

    return node_boolean(lhs->as.number > rhs->as.number);
  }
  case NK_GTE: {
    Node *lhs = node_eval(f->as.ops.lhs, x, y);
    if (!expect_num(lhs)) return NULL;

    Node *rhs = node_eval(f->as.ops.rhs, x, y);
    if (!expect_num(rhs)) return NULL;

    return node_boolean(lhs->as.number >= rhs->as.number);
  }
  case NK_COND: {
    Node *cond = node_eval(f->as.cond.statement, x , y);
    if (!expect_bool(cond)) return NULL;

    Node *iff = node_eval(f->as.cond.iff, x, y);
    if (!iff) return NULL;

    Node *elsee = node_eval(f->as.cond.elsee, x, y);
    if (!elsee) return NULL;

    return cond->as.boolean ? iff : elsee;
  }

  case NK_MOD: {
    Node *lhs = eval(f->as.ops.lhs, x ,y );
    if (!expect_num(lhs)) return NULL;

    Node *rhs = node_eval(f->as.ops.rhs, x, y);
    if (!expect_num(rhs)) return NULL;

    return node_num(fmodf(lhs->as.number, rhs->as.number));
  }

  case RULE_OR: {
    return NULL;
  }
  default:
    printf("UNREACHABLE\n");
    return NULL;
  }
}

void node_print(Node *node)
{
  switch(node->kind) {
  case NK_X:
    printf("x");
    break;
  case NK_Y:
    printf("y");
    break;
  case NK_NUMBER:
    printf("%f", node->as.number);
    break;
  case NK_BOOL:
    printf("%s", node->as.boolean ? "true" : "false");
    break;
  case NK_ADD:
    printf("( ");
    node_print(node->as.ops.lhs);
    printf(", ");
    node_print(node->as.ops.rhs);
    printf(")");
    break;
  case NK_MULT:
    printf("(");
    node_print(node->as.ops.lhs);
    printf("*");
    node_print(node->as.ops.rhs);
    printf(")");
    break;
  case NK_COLOR:
    printf("COLOR(");
    node_print(node->as.color.r);
    printf(", ");
    node_print(node->as.color.g);
    printf(", ");
    node_print(node->as.color.b);
    printf(")");
    break;
  case NK_COND:
    printf("IF ");
    node_print(node->as.cond.statement);
    printf(" THEN ");
    node_print(node->as.cond.iff);
    printf(" ELSE ");
    node_print(node->as.cond.elsee);
    break;
  case NK_GT: {
    printf("( ");
    node_print(node->as.ops.lhs);
    printf(" > ");
    node_print(node->as.ops.rhs);
    printf(")");
    break;
  }
  case NK_GTE: {
    printf("( ");
    node_print(node->as.ops.lhs);
    printf(" >= ");
    node_print(node->as.ops.rhs);
    printf(")");
    break;
  }
  case NK_MOD: {
    printf("MOD(");
    node_print(node->as.ops.lhs);
    printf(", ");
    node_print(node->as.ops.rhs);
    printf(")");
    break;
  }
  case RULE_OR:
  default:
    printf("UNREACHABLE\n");
    exit(1);
    break;
  }
}

bool render_image(Node *f)
{
  for (size_t y = 0; y < HEIGHT; y++) {
    float ny =  (float)y/HEIGHT*2.f - 1;
    for (size_t x = 0; x < WIDTH; x++) {
      float nx =  (float)x/WIDTH*2.f - 1;
      Node *c  = node_eval(f, nx, ny);
      if (!expect_color(c)) return false;
      if (!expect_num(c->as.color.r)) return false;
      if (!expect_num(c->as.color.g)) return false;
      if (!expect_num(c->as.color.b)) return false;

      size_t idx = y*WIDTH+x;

      pixels[idx].r = (c->as.color.r->as.number + 1)/2*255;
      pixels[idx].g = (c->as.color.g->as.number + 1)/2*255;
      pixels[idx].b = (c->as.color.b->as.number + 1)/2*255;
      pixels[idx].a = 255;
    }
  }
  return true;
}

int main()
{
  bool valid = render_image(node_if(
                                    node_gte(node_mult(node_x(), node_y()), node_num(0)),
                                    node_color(
                                               node_x(),
                                               node_y(),
                                               node_num(0)),
                                    node_color(
                                               node_mod(node_x(), node_y()),
                                               node_mod(node_x(), node_y()),
                                               node_mod(node_x(), node_y()))
  ));

  if (!valid) {
    printf("could not generate output.png\n");
    return 1;
  }
  const char *output = "output.png";
  if (!stbi_write_png(output, WIDTH, HEIGHT, 4, pixels, WIDTH*sizeof(RGBA32))) {
    printf("could not generate output.png\n");
    return 1;
  }
  return 0;
}
