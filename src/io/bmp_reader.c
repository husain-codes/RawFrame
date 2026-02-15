#include <io/img_io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

img_t* img_load_bmp(char* path)
{
    BMHEADER header;
    BMINFOHEADER info_header;
    FILE *img_file = fopen(path,"rb");
    if(!img_file)
    {
        fprintf(stderr,"fopen %s \n",path);
        return NULL;
    }

    if(!fread(&header,sizeof(header), 1,img_file))
    {
	fprintf(stderr,"Unable to read header\n");
	fclose(img_file);
        return NULL;
    }

    if(header.signature != 0x4D42)
    {
        fprintf(stderr,"Input file is not bmp file\n");
        fclose(img_file);
        return NULL;
    }

    if(!fread(&info_header,sizeof(info_header), 1,img_file))
    {
        fprintf(stderr,"Unable to read info header\n");
	fclose(img_file);
        return NULL;
    }

    if(info_header.planes != 1)
    {
        fprintf(stderr,"plane should be one\n");
        fclose(img_file);
        return NULL;
    }

    if(info_header.bits_per_pixel != 24)
    {
        fprintf(stderr,"bits_per_pixel must be 24\n");
        fclose(img_file);
        return NULL;
    }

    if (info_header.compression != 0)
    {
        fprintf(stderr, "Compressed BMP not supported\n");
        fclose(img_file);
        return NULL;
    }

    if(fseek(img_file, header.data_offset, SEEK_SET) != 0)
    {
        fprintf(stderr,"Unable to seek to data offset\n");
	fclose(img_file);
	return NULL;
    }
    bool bottom_up = info_header.height > 0 ;
    int height = bottom_up ? info_header.height : -(info_header.height);
    img_t* img = img_create(info_header.width, height, IMG_FMT_RGB24);
    if(!img)
    {
        fprintf(stderr,"Unable to create image\n");
	return NULL;
    }

    int dst_y = 0;
    int bytes_per_row = info_header.width * (info_header.bits_per_pixel / 8);
    int stride = (bytes_per_row + 3) & ~3;
    int padding = stride - bytes_per_row;

    for (int pixel_y=0; pixel_y < height; pixel_y++)
    {
	dst_y = bottom_up ? height - 1 - pixel_y : pixel_y;
        unsigned char* dst_row = img->data + dst_y * bytes_per_row;
        if(fread(dst_row, 1, bytes_per_row, img_file) != bytes_per_row)
	{
	    fprintf(stderr,"Unable to copy raw pixel values at height : %d\n",pixel_y);
	    fclose(img_file);
	    return NULL;
	}
	fseek(img_file,padding,SEEK_CUR);
    }
    fclose(img_file);
    return img;
}

img_t* img_copy(img_t* img)
{

}
