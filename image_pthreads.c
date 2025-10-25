#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
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

typedef struct {
    const Image* src;
    Image* dest;
    Matrix alg;  
    int y0, y1;
} WorkerArgs;

static void* worker(void* arg){
    WorkerArgs* a = (WorkerArgs*)arg;
    const Image* src = a->src;
    Image* dest = a->dest;

    for (int row=a->y0; row<a->y1; ++row){
        for (int pix=0; pix<src->width; ++pix){
            for (int bit=0; bit<src->bpp; ++bit){
                dest->data[Index(pix,row,src->width,bit,src->bpp)] =
                    compute_pixel(src,pix,row,bit,a->alg);
            }
        }
    }
    return NULL;
}

int Usage(){
    printf("Usage: image_pthreads <filename> <type> [threads]\n"
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

    if (argc<3 || argc>4) return Usage();
    char* fileName=argv[1];
    enum KernelTypes type=GetKernelType(argv[2]);

    int threads = (argc==4 ? atoi(argv[3]) : 4);
    if (threads <= 0) threads = 4;

    Image src,dst;
    stbi_set_flip_vertically_on_load(0);
    src.data = stbi_load(fileName,&src.width,&src.height,&src.bpp,0);
    if (!src.data){ fprintf(stderr,"Error loading file %s.\n", fileName); return -1; }

    dst.width = src.width; dst.height = src.height; dst.bpp = src.bpp;
    dst.data = (uint8_t*)malloc((size_t)dst.width*dst.height*dst.bpp);
    if (!dst.data){ fprintf(stderr,"malloc failed\n"); return -1; }

    pthread_t *t = (pthread_t*)malloc(sizeof(pthread_t)*threads);
    WorkerArgs *args = (WorkerArgs*)malloc(sizeof(WorkerArgs)*threads);

    int rows = src.height;
    int base = rows / threads, rem = rows % threads, y=0;
    for (int i=0;i<threads;i++){
        int take = base + (i<rem ? 1:0);
        args[i].src=&src; args[i].dest=&dst;
        for(int r=0;r<3;r++) for(int c=0;c<3;c++) args[i].alg[r][c]=algorithms[type][r][c];
        args[i].y0=y; args[i].y1=y+take; y+=take;
        pthread_create(&t[i], NULL, worker, &args[i]);
    }
    for (int i=0;i<threads;i++) pthread_join(t[i], NULL);

    stbi_write_png("output.png",dst.width,dst.height,dst.bpp,dst.data,dst.bpp*dst.width);

    stbi_image_free(src.data);
    free(dst.data);
    free(t); free(args);

    time_t t2=time(NULL);
    printf("pthread version took %ld seconds with %d threads\n",(long)(t2-t1),threads);
    return 0;
}
