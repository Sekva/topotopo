#ifndef MAIN_H
#define MAIN_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <Imlib2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

#define NUM_PIXMAPS 2

const char* ClassName = "topotopo";
char* ProgramName;
Display* display;
char* display_name;
int screen;
Window root;
int depth;
int win_mask;
XSetWindowAttributes win_attrib;
XWindowAttributes attrib;
XVisualInfo *xvi;
XVisualInfo vi_in;
int nvi, i, scr = 0;
Visual *vis;

Window window;
Window foco;
XWindowAttributes attrib_foco;
GC contexto;

int pos_x, pos_y;
int tamanho = 50;

unsigned long _RGBA(int r,int g, int b, int a) {
    return a + (b << 8) + (g<<16) + (r<<24);
}
unsigned long RGBA(int r,int g, int b, int a) {
    return b + (g << 8) + (r<<16) + (a<<24);
}
unsigned long RGB(int r,int g, int b) {
    return _RGBA(255, r, g, b);
}

int trocar_pixmap = 0;
int mover_pelo_mouse = 0;
Pixmap pixmap_atual;
Pixmap pixmaps[NUM_PIXMAPS];

typedef struct Pixel {
    unsigned int x;
    unsigned int y;
    unsigned long cor_rgba;
} Pixel;

typedef struct Imagem {
    unsigned int altura;
    unsigned int largura;
    unsigned int num_pixels;
    Pixel* pixels;
} Imagem;



#endif
