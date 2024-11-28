#include <X11/X.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define MEDIA 25
#define TAMANHO (MEDIA * 2)
#define VELOCIDADE 20

#define QPS 20
#define SKIPS 3
#define QUADROS 10
#define FRAMETIME_MS 16

#define RGB(r, g, b)                                                           \
  (0xFF000000 | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF)))

typedef struct {
  double x;
  double y;
} Vec2;

Vec2 pegar_janela(Display *display, Window root, bool quina) {
  int revert_to;
  Window foco;
  XGetInputFocus(display, &foco, &revert_to);
  int x, y;
  Window child;
  XTranslateCoordinates(display, foco, root, 0, 0, &x, &y, &child);
  XWindowAttributes attrib_foco;
  XGetWindowAttributes(display, foco, &attrib_foco);
  x = x - attrib_foco.x;
  y = y - attrib_foco.y;

  int offset_x = MEDIA, offset_y = MEDIA;

  if (quina) {
    offset_x += attrib_foco.width;
    offset_y += attrib_foco.height;
  }

  int pos_x = (x + offset_x) - TAMANHO + 1;
  int pos_y = (y + offset_y) - TAMANHO + 1;
  return (Vec2){(int)pos_x, (int)pos_y};
}

void atualizar_posicao(Display *display, Window window, Vec2 pos) {
  XWindowChanges theChanges;
  theChanges.x = (int)pos.x;
  theChanges.y = (int)pos.y;
  XConfigureWindow(display, window, CWX | CWY, &theChanges);
}

double calcular_distancia(Vec2 a, Vec2 b) {
  return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

Vec2 calcular_direcao(Vec2 ponto, Vec2 alvo) {
  double nx = (alvo.x - ponto.x);
  double ny = (alvo.y - ponto.y);
  return (Vec2){nx, ny};
}

void escalar_vec2(double escalar, Vec2 *p) {
  p->x *= escalar;
  p->y *= escalar;
}

void print_vec2(Vec2 v, const char *label) {
  printf("%s: (%lf, %lf)\n", label, v.x, v.y);
}

void normalizar_vec2(Vec2 *p) {
  double norma = calcular_distancia((Vec2){0, 0}, *p);
  if (norma < 0.0001) {
    p->x = 0.0;
    p->y = 0.0;
  } else {
    p->x = p->x / norma;
    p->y = p->y / norma;
  }
}

void mover(Vec2 *ponto, Vec2 movimento) {
  ponto->x += movimento.x;
  ponto->y += movimento.y;
}

Pixmap circulo_pixmap(Display *display, Drawable drawable, int tamanho,
                      unsigned long cor_fundo, unsigned long cor_circulo,
                      unsigned long margem) {
  Pixmap pixmap = XCreatePixmap(display, drawable, tamanho, tamanho, 32);
  GC gc = XCreateGC(display, pixmap, 0, NULL);
  XSetForeground(display, gc, cor_fundo);
  XFillRectangle(display, pixmap, gc, 0, 0, tamanho, tamanho);
  XSetForeground(display, gc, cor_circulo);

  int x = margem - 5;
  int y = margem - 5;

  int width = tamanho - (2 * margem);
  int height = tamanho - (2 * margem);

  XFillArc(display, pixmap, gc, x, y, width, height, 0, 360 * 64);

  XFreeGC(display, gc);
  return pixmap;
}

void criar_animacao(Display *display, Window window, Pixmap *pixmaps,
                    int quadros) {
  for (int i = 0; i < quadros; i++) {
    int margem = 5 + i * 2;
    unsigned long cor_circulo = 0xFF0000FF + (i * 0x0000FF00);
    pixmaps[i] = circulo_pixmap(display, window, TAMANHO, 0x00000000,
                                cor_circulo, margem);
  }
}

bool verificar_mouse(Display *display, Window topo, Vec2 pos_atual,
                     bool movendo) {

  if (movendo) {
    return false;
  }

  int mouse_x, mouse_y;

  Window _t1, _t2;
  int _it1, _it2;
  unsigned int _it3;

  bool mudar_quina = false;

  XQueryPointer(display, topo, &_t1, &_t2, &mouse_x, &mouse_y, &_it1, &_it2,
                &_it3);

  int seguranca = 10;

  int teste_x = mouse_x >= (pos_atual.x - seguranca) &&
                mouse_x <= (pos_atual.x + TAMANHO + seguranca);
  int teste_y = mouse_y >= (pos_atual.y - seguranca) &&
                mouse_y <= (pos_atual.y + TAMANHO + seguranca);

  if (teste_x && teste_y) {
    mudar_quina = true;
  }

  return mudar_quina;
}

int main(void) {
  Display *display = XOpenDisplay(NULL);
  int screen = XDefaultScreen(display);
  Window root = RootWindow(display, screen);
  int win_mask = CWBackPixel | CWCursor | CWOverrideRedirect | CWColormap |
                 CWEventMask | CWBackPixmap | CWBorderPixel;

  XVisualInfo visualinfo;
  XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &visualinfo);

  XSetWindowAttributes win_attrib = {0};
  win_attrib.colormap =
      XCreateColormap(display, root, visualinfo.visual, AllocNone);
  win_attrib.event_mask = ExposureMask;
  win_attrib.background_pixmap = None;
  win_attrib.override_redirect = True;
  win_attrib.border_pixel = 0;
  win_attrib.background_pixel = 0xFF000000;

  bool movendo = true;
  bool quina = true;
  int quadro_atual = 0;
  int contador = 1;
  Vec2 pos_alvo = pegar_janela(display, root, quina);
  Vec2 pos_atual = pos_alvo;

  Window topo = XCreateWindow(
      display, root, (int)pos_atual.x, (int)pos_atual.y, TAMANHO, TAMANHO, 0,
      visualinfo.depth, InputOutput, visualinfo.visual, win_mask, &win_attrib);
  XSelectInput(display, topo, StructureNotifyMask);

  GC contexto = XCreateGC(display, topo, 0, 0);

  Pixmap pixmaps[QUADROS];
  criar_animacao(display, topo, pixmaps, QUADROS);
  XSetWindowBackgroundPixmap(display, topo, pixmaps[0]);

  XStoreName(display, topo, "topotopo");
  XMapWindow(display, topo);
  XSetBackground(display, contexto, 0L);
  XFlush(display);

  while (1) {

    pos_alvo = pegar_janela(display, root, quina);
    double distancia = calcular_distancia(pos_alvo, pos_atual);
    Vec2 direcao = calcular_direcao(pos_atual, pos_alvo);
    normalizar_vec2(&direcao);

    double escalar = 6;

    while (escalar > distancia) {
      escalar = escalar / 2;
    }

    if (distancia > 1) {
      escalar_vec2(escalar, &direcao);
      mover(&pos_atual, direcao);

      if (!movendo) {
        movendo = true;
        /* printf("movendo\n"); */
      }
    } else {
      if (movendo) {
        movendo = false;
        /* printf("parou\n"); */
      }
    }

    if (contador == 0) {
      quadro_atual = (quadro_atual + 1) % QUADROS;
      XSetWindowBackgroundPixmap(display, topo, pixmaps[quadro_atual]);
      XClearWindow(display, topo);
    }

    atualizar_posicao(display, topo, pos_atual);
    bool mudar_quina = verificar_mouse(display, topo, pos_atual, movendo);

    if (mudar_quina) {
      /* printf("mudar quina\n"); */
      quina = !quina;
    }

    XFlush(display);
    usleep(FRAMETIME_MS * 1000);
    contador = (contador + 1) % SKIPS;
  }

  for (int i = 0; i < QUADROS; i++) {
    XFreePixmap(display, pixmaps[i]);
  }
  XDestroyWindow(display, topo);
  XCloseDisplay(display);

  return 0;
}
