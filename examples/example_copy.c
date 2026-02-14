#include <stdio.h>
#include <img/img.h>

int main()
{
    printf("Examples\n");
    img_t* image = img_create(1200,900,IMG_FMT_RGB24);
    printf("create image of resolution %d x %d with stride %d\n",image->width,image->height,image->stride);
    img_destroy(image);
    return 0;
}
