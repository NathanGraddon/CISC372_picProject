#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Matrix algorithms[] = {
    {{0,-1,0},{-1,4,-1},{0,-1,0}},
    {{0,-1,0},{-1,5,-1},{0,-1,0}},
    {{1/9.0,1/9.0,1/9.0},{1/9.0,1/9.0,1/9.0},{1/9.0,1/9.0,1/9.0}},
    {{1.0/16,1.0/8,1.0/16},{1.0/8,1.0/4,1.0/8},{1.0/16,1.0/8,1.0/16}},
    {{-2,-1,0},{-1,1,1},{0,1,2}},
    {{0,0,0},{0,1,0},{0,0,0}}
};

static inline uint8_t compute_pixel(const Image* src, int x, int y, int bit, Matrix alg){
    int px=x+1, mx=x-1, py=y+1, my=y-1;
    if (mx<0) mx=0;
    if (my<0) my=0;
    if (px>=src->width)  px=src->width-1;
    if (py>=src->height) py=src->height-1;

    double v =
      alg[0][0]*src->data[Index(mx,my,src->width,bit,src->bpp)] +
      alg[0][1]*src->data[Index(x ,my,src->width,bit,src->bpp)] +
      alg[0][2]*src->data[Index(px,my,src->width,bit,src->bpp)] +
      alg[1][0]*src->data[Index(mx,y ,src->width,bit,src->bpp)] +
      alg[1][1]*src->data[Index(x ,y ,src->width,bit,src->bpp)] +
      alg[1][2]*src->data[Index(px,y ,src->width,bit,src->bpp)] +
      alg[2][0]*src->data[Index(mx,py,src->width,bit,src->bpp)] +
      alg[2][1]*src->data[Index(x ,py,src->width,bit,src->bpp)] +
      alg[2][2]*src->data[Index(px,py,src->width,bit,src->bpp)];

    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return (uint8_t)(v + 0.5);
}

int Usage(){
    printf("Usage: image_omp <filename> <type>\n"
           "\twhere type is one of (edge,sharpen,blur,gauss,emboss,identity)\n");
    return -1;
}

enum KernelTypes GetKernelType(char* type){
    if (!strcmp(type,"edge")) return EDGE;
    else if (!strcmp(type,"sharpen")) return SHARPEN;
    else if (!strcmp(type,"blur")) return BLUR;
    else if (!strcmp(type,"gauss")) return GAUSE_BLUR;
    else if (!strcmp(type,"emboss")) return EMBOSS;
    else return IDENTITY;
}

int main(int argc, char** argv){
    time_t t1=time(NULL);

    if (argc!=3) return Usage();
    char* fileName=argv[1];
    enum KernelTypes type=GetKernelType(argv[2]);

    Image src,dst;
    stbi_set_flip_vertically_on_load(0);
    src.data = stbi_load(fileName,&src.width,&src.height,&src.bpp,0);
    if (!src.data){ fprintf(stderr,"Error loading file %s.\n", fileName); return -1; }

    dst.width = src.width; dst.height = src.height; dst.bpp = src.bpp;
    dst.data = (uint8_t*)malloc((size_t)dst.width*dst.height*dst.bpp);
    if (!dst.data){ fprintf(stderr,"malloc failed\n"); return -1; }

    Matrix alg = {0};
    for (int r=0;r<3;r++) for (int c=0;c<3;c++) alg[r][c]=algorithms[type][r][c];

    #pragma omp parallel for schedule(static)
    for (int row=0; row<src.height; ++row){
        for (int pix=0; pix<src.width; ++pix){
            for (int bit=0; bit<src.bpp; ++bit){
                dst.data[Index(pix,row,src.width,bit,src.bpp)] =
                    compute_pixel(&src,pix,row,bit,alg);
            }
        }
    }

    stbi_write_png("output.png",dst.width,dst.height,dst.bpp,dst.data,dst.bpp*dst.width);

    stbi_image_free(src.data);
    free(dst.data);

    time_t t2=time(NULL);
    printf("OpenMP version took %ld seconds (threads=%d)\n",
           (long)(t2-t1), omp_get_max_threads());
    return 0;
}
