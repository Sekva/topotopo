#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint32_t metade_imagem;
uint32_t tamanho_imagem;

#define VELOCIDADE 20
#define QPS 20
#define SKIPS 3
#define FRAMETIME_MS 16
#define MAX_IMAGENS 1024

#define RGBA(r, g, b, a)                                                       \
  (((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF)))

void *sek_memcpy(void *dest, const void *src, int n) {

  uint32_t *d = dest;
  const uint32_t *s = src;
  n = n / 4;

  for (; n; n--) {

    char r = ((*s) >> 0) & 0xFF;
    char g = ((*s) >> 8) & 0xFF;
    char b = ((*s) >> 16) & 0xFF;
    char a = ((*s) >> 24) & 0xFF;

    *d++ = RGBA(r, g, b, a);

    s++;
  }
  return dest;
}

typedef struct {
  double x;
  double y;
} Vec2;

typedef struct {
  Pixmap *pixmaps;
  int quantidade;
  uint32_t tamanho; // assume quadrado
} PixmapList;

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

  int offset_x = metade_imagem, offset_y = metade_imagem;

  if (quina) {
    offset_x += attrib_foco.width;
    offset_y += attrib_foco.height;
  }

  int pos_x = (x + offset_x) - tamanho_imagem + 1;
  int pos_y = (y + offset_y) - tamanho_imagem + 1;
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
                mouse_x <= (pos_atual.x + tamanho_imagem + seguranca);
  int teste_y = mouse_y >= (pos_atual.y - seguranca) &&
                mouse_y <= (pos_atual.y + tamanho_imagem + seguranca);

  if (teste_x && teste_y) {
    mudar_quina = true;
  }

  return mudar_quina;
}

XImage *resize_ximage(Display *display, XImage *src, int target_width,
                      int target_height) {

  XImage *resized = XCreateImage(
      display, DefaultVisual(display, DefaultScreen(display)), src->depth,
      ZPixmap, 0, NULL, target_width, target_height, 32, 0);

  resized->data = (char *)malloc(target_height * resized->bytes_per_line);

  double scale_x = (double)src->width / target_width;
  double scale_y = (double)src->height / target_height;

  for (int y = 0; y < target_height; y++) {
    for (int x = 0; x < target_width; x++) {
#if defined(VIZINHO)
      int src_x = (int)(x * scale_x);
      int src_y = (int)(y * scale_y);
      XPutPixel(resized, x, y, XGetPixel(src, src_x, src_y));
#elif defined(BILINEAR)
      // Coordenadas no espaço da imagem original
      double src_x = x * scale_x;
      double src_y = y * scale_y;

      // Coordenadas dos pixels ao redor
      int x0 = (int)src_x;
      int x1 = x0 + 1 < src->width ? x0 + 1 : x0;
      int y0 = (int)src_y;
      int y1 = y0 + 1 < src->height ? y0 + 1 : y0;

      // Frações para interpolação
      double dx = src_x - x0;
      double dy = src_y - y0;

      // Valores dos 4 pixels ao redor
      uint32_t p00 = XGetPixel(src, x0, y0);
      uint32_t p01 = XGetPixel(src, x0, y1);
      uint32_t p10 = XGetPixel(src, x1, y0);
      uint32_t p11 = XGetPixel(src, x1, y1);

      // Interpolar cores (R, G, B, A separadamente)
      uint8_t r = (1 - dx) * (1 - dy) * ((p00 >> 16) & 0xFF) +
                  dx * (1 - dy) * ((p10 >> 16) & 0xFF) +
                  (1 - dx) * dy * ((p01 >> 16) & 0xFF) +
                  dx * dy * ((p11 >> 16) & 0xFF);

      uint8_t g = (1 - dx) * (1 - dy) * ((p00 >> 8) & 0xFF) +
                  dx * (1 - dy) * ((p10 >> 8) & 0xFF) +
                  (1 - dx) * dy * ((p01 >> 8) & 0xFF) +
                  dx * dy * ((p11 >> 8) & 0xFF);

      uint8_t b = (1 - dx) * (1 - dy) * (p00 & 0xFF) +
                  dx * (1 - dy) * (p10 & 0xFF) + (1 - dx) * dy * (p01 & 0xFF) +
                  dx * dy * (p11 & 0xFF);

      uint8_t a = (1 - dx) * (1 - dy) * ((p00 >> 24) & 0xFF) +
                  dx * (1 - dy) * ((p10 >> 24) & 0xFF) +
                  (1 - dx) * dy * ((p01 >> 24) & 0xFF) +
                  dx * dy * ((p11 >> 24) & 0xFF);

      // Combinar os componentes em um único pixel
      uint32_t pixel = (a << 24) | (r << 16) | (g << 8) | b;

      // Colocar o pixel na nova imagem
      XPutPixel(resized, x, y, pixel);
#else
      assert(0 && "Pelo menos VIZINHO ou BILINEAR tem q ser definido");
#endif
    }
  }

  return resized;
}

Pixmap carregar_pixmap_de_arquivo(Display *display, Window root,
                                  const char *file_path, int largura,
                                  int altura) {

  FILE *fp = fopen(file_path, "rb");
  if (!fp) {
    fprintf(stderr, "Error: Unable to open file %s\n", file_path);
    return None;
  }

  // Leitura inicial do cabeçalho do PNG
  unsigned char header[8];
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "Error: File %s is not a valid PNG file\n", file_path);
    fclose(fp);
    return None;
  }

  // Inicializar estruturas do libpng
  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf(stderr, "Error: Unable to create PNG read struct\n");
    fclose(fp);
    return None;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    fprintf(stderr, "Error: Unable to create PNG info struct\n");
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return None;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    fprintf(stderr, "Error: libpng error during init_io\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return None;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  // Obter dimensões da imagem
  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  // Ler pixels da imagem
  png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
  }

  png_read_image(png_ptr, row_pointers);

  fclose(fp);

  XImage *base_ximage =
      XCreateImage(display, DefaultVisual(display, DefaultScreen(display)), 32,
                   ZPixmap, 0, NULL, width, height, 32, 0);

  base_ximage->data = (char *)malloc(height * base_ximage->bytes_per_line);
  for (int y = 0; y < height; y++) {
    sek_memcpy(base_ximage->data + y * base_ximage->bytes_per_line,
               row_pointers[y], png_get_rowbytes(png_ptr, info_ptr));
    free(row_pointers[y]);
  }
  free(row_pointers);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  XImage *ximage = resize_ximage(display, base_ximage, largura, altura);
  XDestroyImage(base_ximage);

  tamanho_imagem =
      ximage->width > ximage->height ? ximage->width : ximage->height;
  metade_imagem = tamanho_imagem / 2;

  Pixmap pixmap = XCreatePixmap(display, root, width, height, 32);

  GC gc = XCreateGC(display, pixmap, 0, NULL);
  XPutImage(display, pixmap, gc, ximage, 0, 0, 0, 0, width, height);

  XDestroyImage(ximage);
  XFreeGC(display, gc);

  return pixmap;
}

PixmapList carregar_imagens(Display *display, Window topotopo, int largura,
                            int altura) {
  DIR *d;
  struct dirent *dir;
  d = opendir("ayaya");
  int n_imagens = 0;

  const char arquivos[MAX_IMAGENS][256 + sizeof("ayaya/")] = {0};

  if (d) {
    while ((dir = readdir(d)) != NULL) {

      if (n_imagens == MAX_IMAGENS) {
        break;
      }

      if (dir->d_type != 8) {
        continue;
      }

      strcpy((char *)(arquivos[n_imagens]), "ayaya/");
      strcpy((char *)(arquivos[n_imagens] + sizeof("ayaya/") - 1), dir->d_name);

      n_imagens++;
    }
    closedir(d);
  }

  PixmapList pixmaps = {0};
  pixmaps.quantidade = n_imagens;
  pixmaps.pixmaps = (Pixmap *)calloc(n_imagens, sizeof(Pixmap));

  for (int i = 0; i < pixmaps.quantidade; i++) {
    pixmaps.pixmaps[i] = carregar_pixmap_de_arquivo(
        display, topotopo, arquivos[i], largura, altura);
  }

  return pixmaps;
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

  PixmapList pixmaps = carregar_imagens(display, root, 50, 50);

  Window topo =
      XCreateWindow(display, root, (int)pos_atual.x, (int)pos_atual.y,
                    tamanho_imagem, tamanho_imagem, 0, 32, InputOutput,
                    visualinfo.visual, win_mask, &win_attrib);
  XSelectInput(display, topo, StructureNotifyMask);

  GC contexto = XCreateGC(display, topo, 0, 0);

  XSetWindowBackgroundPixmap(display, topo, pixmaps.pixmaps[0]);

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
      quadro_atual = (quadro_atual + 1) % pixmaps.quantidade;
      XSetWindowBackgroundPixmap(display, topo, pixmaps.pixmaps[quadro_atual]);
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
    /* break; */
  }

  for (int i = 0; i < pixmaps.quantidade; i++) {
    XFreePixmap(display, pixmaps.pixmaps[i]);
  }

  free(pixmaps.pixmaps);
  XDestroyWindow(display, topo);
  XCloseDisplay(display);

  return 0;
}
