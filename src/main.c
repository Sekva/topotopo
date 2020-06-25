#include "main.h"
#include <Imlib2.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
void iniciar_pixmaps() {

    // Os frames q vai ser usado
    const char* nomes[NUM_PIXMAPS];
    nomes[0] = "img50.png";
    nomes[1] = "img50r.png";

    // Cada pixmap é um frame
    for(int i = 0; i < NUM_PIXMAPS; i++) {

        // Criar a imagem crua
        Imagem ayaya;
        ayaya.altura = tamanho;
        ayaya.largura = tamanho;
        ayaya.pixels = (Pixel*) calloc(tamanho * tamanho, sizeof(Pixel));
        ayaya.num_pixels= tamanho * tamanho;
        Pixmap t_pixmap = XCreatePixmap(display, window, tamanho, tamanho, depth);

        // Limpa a pixmap
        for(int x = 0; x < tamanho; x++) {
            for(int y = 0; y < tamanho; y++) {
                XSetForeground(display, contexto, 0L);
                XDrawPoint(display, t_pixmap, contexto, x, y);
            }
        }

        // Carrega a img
        Imlib_Image imagem_imlib = imlib_load_image(nomes[i]);
        // Coloca a imagem no contexto
        imlib_context_set_image(imagem_imlib);

        int contador_pixel = 0;
        for(int x = 0; x < tamanho; x++) {
            for(int y = 0; y < tamanho; y++) {
                Imlib_Color cor_imlib;
                imlib_image_query_pixel(x, y, &cor_imlib);

                if(cor_imlib.alpha != 0) {
                    unsigned long cor_ul = RGBA(cor_imlib.red, cor_imlib.green, cor_imlib.blue, cor_imlib.alpha);
                    ayaya.pixels[contador_pixel].cor_rgba = cor_ul;
                    ayaya.pixels[contador_pixel].x = x;
                    ayaya.pixels[contador_pixel].y = y;
                    contador_pixel += 1;
                }
            }
        }

        ayaya.pixels = realloc(ayaya.pixels, contador_pixel * sizeof(Pixel));
        ayaya.num_pixels = contador_pixel;

        //Dá aquele free
        imlib_free_image();


        //Copia os pixels puros pro pixmap
        for(int i = 0; i < ayaya.num_pixels; i++) {
                unsigned long cor = ayaya.pixels[i].cor_rgba;
                unsigned long x = ayaya.pixels[i].x;
                unsigned long y = ayaya.pixels[i].y;
                XSetForeground(display, contexto, cor);
                XDrawPoint(display, t_pixmap, contexto, x, y);
        }

        pixmaps[i] = t_pixmap;
    }
}

void pegar_janela() {

    int revert_to;
    XGetInputFocus(display, &foco, &revert_to);

    int x, y;
    Window child;
    XTranslateCoordinates(display,foco, root, 0, 0, &x, &y, &child);
    XGetWindowAttributes(display, foco, &attrib_foco);

    x = x - attrib_foco.x;
    y = y - attrib_foco.y;

    if(mover_pelo_mouse) {
        pos_x = x;
        pos_y = y;
    } else {
        pos_x = (x + attrib_foco.width) - tamanho + 1;
        pos_y = (y + attrib_foco.height) - tamanho + 1;
    }
}

void copiar_bg() {

    int frame_x = (attrib_foco.x + attrib_foco.width) - tamanho - 1;
    int frame_y = (attrib_foco.y + attrib_foco.height) - tamanho - 1;

    XImage* img = XGetImage(display, foco, frame_x, frame_y, tamanho, tamanho, AllPlanes, ZPixmap);

    if(!img) {
        printf("deu ruim pra pegar o bg\n");
        exit(2);
    }

    for(int x = 0; x < img->width; x++) {
        for(int y = 0; y < img->height; y++) {
            unsigned long val = XGetPixel(img, x, y);
            XSetForeground(display, contexto, val);
            XDrawPoint(display, window, contexto,x,y);
        }
    }
}

void atualizar_janela() {
    XWindowChanges theChanges;
    theChanges.x = pos_x;
    theChanges.y = pos_y;
    XConfigureWindow(display,window, CWX | CWY, &theChanges);
}

void desenhar() {
    if(trocar_pixmap) {
        XCopyArea(display, pixmap_atual, window, contexto, 0, 0, tamanho, tamanho, 0, 0);
        trocar_pixmap = 0;
    }
}

void verificar_mouse() {
    int mouse_x, mouse_y;

    Window _t1, _t2;
    int _it1, _it2;
    unsigned int _it3;

    XQueryPointer(display, window, &_t1, &_t2, &mouse_x, &mouse_y, &_it1, &_it2, &_it3);

    int seguranca = 10;

    int teste_x = mouse_x >= (pos_x - seguranca) && mouse_x <= (pos_x + tamanho + seguranca);
    int teste_y = mouse_y >= (pos_y - seguranca) && mouse_y <= (pos_y + tamanho + seguranca);

    if(teste_x && teste_y) {
        mover_pelo_mouse = !mover_pelo_mouse;
    }

}


int main () {
    display = XOpenDisplay(NULL);
    screen = XDefaultScreen(display);
    root = RootWindow(display, screen);
    depth = 32;//XDefaultDepth(display, screen);
    win_mask = CWBackPixel | CWCursor | CWOverrideRedirect | CWColormap | CWEventMask | CWBackPixmap | CWBorderPixel;
    pos_x = pos_y = 0;

    XVisualInfo visualinfo ;
    XMatchVisualInfo(display, DefaultScreen(display), depth, TrueColor, &visualinfo);
    win_attrib.colormap   = XCreateColormap( display, root, visualinfo.visual, AllocNone) ;
    win_attrib.event_mask = ExposureMask;
    win_attrib.background_pixmap = None ;
    win_attrib.override_redirect = True;
    win_attrib.border_pixel      = 0 ;

    window = XCreateWindow(display,root, pos_x, pos_y, tamanho, tamanho, 0, visualinfo.depth, InputOutput, visualinfo.visual, win_mask, &win_attrib);
    contexto = XCreateGC( display, window, 0, 0);
    XStoreName( display, window, ClassName);
    XMapWindow(display, window);
    XSetBackground(display, contexto, 0L);
    XFlush(display);

    iniciar_pixmaps();
    pixmap_atual = pixmaps[0];

    int contador = 0;
    int pixmap_idx = 0;
    while(++contador) {
        if(contador == 60) {
            contador = 0;
            trocar_pixmap = 1;
            pixmap_idx = (pixmap_idx + 1) % NUM_PIXMAPS;
            pixmap_atual = pixmaps[pixmap_idx];
        }
        verificar_mouse();
        pegar_janela();
        atualizar_janela();
        desenhar();
        XFlush(display);
        usleep(15000);
    }

    return 0;
}
