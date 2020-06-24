#!/bin/python3
from PIL import Image

im_ayaya = Image.open('img50.png')
ayaya = im_ayaya.load()

(largura_ayaya, altura_ayaya) = im_ayaya.size

string = "\nImagem ayaya;\n"
string += "ayaya.altura = " + str(altura_ayaya) + ";\n"
string += "ayaya.largura = " + str(largura_ayaya)  + ";\n"

string += "Pixel* temp = malloc(" + str(largura_ayaya * altura_ayaya) + " * sizeof(Pixel));\n"
i = 1;
for x in range(largura_ayaya):
    for y in range(altura_ayaya):
        (r, g, b, a) = ayaya[x, y]
        if a == 0:
            continue
        cc = "RGB(" + str(r) +  ", " + str(g) + ", " + str(b) + ")"
        string += "temp[" + str(i-1) + "] = (Pixel){ " + str(x) + ", " + str(y) + ", " + cc + " };\n"
        i += 1

string += "ayaya.pixels = temp;\n"
string += "ayaya.num_pixels= "+ str(i-1) + ";\n"
string += """
ayaya_pixmap = XCreatePixmap(display, window, tamanho, tamanho, depth);

for(int x = 0; x < tamanho; x++) {
    for(int y = 0; y < tamanho; y++) {
        XSetForeground(display, contexto, 0L);
        XDrawPoint(display, ayaya_pixmap, contexto, x, y);
    }
}

for(int i = 0; i < ayaya.num_pixels; i++) {
    unsigned long cor = ayaya.pixels[i].cor_rgba;
    unsigned long x = ayaya.pixels[i].x;
    unsigned long y = ayaya.pixels[i].y;
    XSetForeground(display, contexto, cor);
    XDrawPoint(display, ayaya_pixmap, contexto, x, y);
}



///////////////////////////////////////////////////////////////








"""


im_ayaya = Image.open('img50r.png')
ayaya = im_ayaya.load()

(largura_ayaya, altura_ayaya) = im_ayaya.size

string += "\nImagem ayaya_rodada;\n"
string += "ayaya_rodada.altura = " + str(altura_ayaya) + ";\n"
string += "ayaya_rodada.largura = " + str(largura_ayaya)  + ";\n"

string += "Pixel* temp2 = malloc(" + str(largura_ayaya * altura_ayaya) + " * sizeof(Pixel));\n"
i = 1;
for x in range(largura_ayaya):
    for y in range(altura_ayaya):
        (r, g, b, a) = ayaya[x, y]
        if a == 0:
            continue
        cc = "RGB(" + str(r) +  ", " + str(g) + ", " + str(b) + ")"
        string += "temp2[" + str(i-1) + "] = (Pixel){ " + str(x) + ", " + str(y) + ", " + cc + " };\n"
        i += 1

string += "ayaya_rodada.pixels = temp2;\n"
string += "ayaya_rodada.num_pixels= "+ str(i-1) + ";\n"
string += """
ayaya_rodada_pixmap = XCreatePixmap(display, window, tamanho, tamanho, depth);

for(int x = 0; x < tamanho; x++) {
    for(int y = 0; y < tamanho; y++) {
        XSetForeground(display, contexto, 0L);
        XDrawPoint(display, ayaya_pixmap, contexto, x, y);
    }
}

for(int i = 0; i < ayaya_rodada.num_pixels; i++) {
    unsigned long cor = ayaya_rodada.pixels[i].cor_rgba;
    unsigned long x = ayaya_rodada.pixels[i].x;
    unsigned long y = ayaya_rodada.pixels[i].y;
    XSetForeground(display, contexto, cor);
    XDrawPoint(display, ayaya_rodada_pixmap, contexto, x, y);
}

"""





print(string)
