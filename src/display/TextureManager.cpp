#include "TextureManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>

#include <png.h>
// #include <jpeglib.h>

#ifndef ABS
#define ABS(x) (((x) < 0)?-(x):(x))
#endif

#define glError() {                \
    GLenum err = glGetError();                                          \
    while (err != GL_NO_ERROR) {                                        \
      fprintf(stderr, "glError: %s caught at %s:%u\n", (char *)gluErrorString(err), __FILE__, __LINE__); \
      err = glGetError();                                               \
    }                                                                   \
  }

TextureManager::TextureManager()
{

}

TextureManager::~TextureManager()
{

}

int TextureManager::buildColorTexture(GLuint *TID, unsigned char r, unsigned char g, unsigned char b)
{
  unsigned char data[12];	// a 2x2 texture at 24 bits

  // Store the data
  for(int i = 0; i < 12; i += 3) {
    data[i] = r;
    data[i+1] = g;
    data[i+2] = b;
  }

  // Generate the OpenGL texture id
  glGenTextures(1, TID);
  
  // Bind this texture to its id
  glBindTexture(GL_TEXTURE_2D, *TID);
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  // Use mipmapping filter
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  
  // Generate the texture
  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 2, 2, GL_RGB, GL_UNSIGNED_BYTE, data);
  //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 3, 3, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

  return 0;
}

int TextureManager::loadTexture(GLuint *TID, char *filename, image_type_t type, bool wrap)
{
  FILE *file;
  image_t image;
  int result = -1;

  if (!TID) return -1;

  // open texture data
  file = fopen(filename, "rb");
  if (file == NULL) return -1;
  switch (type) {
  case RAW_IMAGE:
    result = loadRAWImage(filename, file, 256, 256, &image);
    break;
  case BMP_IMAGE:
    result = loadBMPImage(filename, file, &image);
    break;
  case BMP_ALPHA_IMAGE:
    result = loadBMPAlphaImage(filename, file, &image);
    break;
  case PNG_IMAGE:
    result = loadPNGImage(filename, file, &image);
    break;
  case JPG_IMAGE:
    result = loadJPGImage(filename, file, &image);
    break;
  }
  fclose(file);
  if (result < 0) return -1;

  // allocate a texture name
  glGenTextures(1, TID);

  // select our current texture
  glBindTexture(GL_TEXTURE_2D, *TID);

  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP);

  if (result == 1)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.sizeX, image.sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.sizeX, image.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);

  // free buffer
  free(image.data);

  return 0;
}

/**
 * Code taken from: http://zarb.org/~gc/html/libpng.html
 */
int TextureManager::loadPNGImage(char *filename, FILE *fp, image_t *image)
{
  int x, y;
  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  png_structp png_ptr;
  png_infop info_ptr;
  int number_of_passes;
  png_bytep * row_pointers;

  char header[8];    // 8 is the maximum size that can be checked

  /* READ PNG */

  /* test for it being a png */
  fread(header, 1, 8, fp);
  if (png_sig_cmp((png_byte*)header, 0, 8)) return -1;

  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) return -1;

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) return -1;

  if (setjmp(png_jmpbuf(png_ptr))) return -1;

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  
  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  /* read file */
  if (setjmp(png_jmpbuf(png_ptr))) return -1;

  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
  for (y=0; y<height; y++) row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
  png_read_image(png_ptr, row_pointers);

  /* PROCESS */

  int res = 0;
  if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA) res = 1;
  else if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGB) {
    for (y=0; y<height; y++) free(row_pointers[y]);
    free(row_pointers);
    return -1;
  }
  
  image->data = (unsigned char *) malloc(height*width*(3+res));
  if (!image->data) {
    for (y=0; y<height; y++) free(row_pointers[y]);
    free(row_pointers);
    return -1;
  }

  int cnt = 0;
  for (y=height-1; y>=0; y--) {
    png_byte* row = row_pointers[y];
    for (x=0; x<width; x++) {
      png_byte* ptr = &(row[x*(3+res)]);

      image->data[cnt++] = ptr[0];
      image->data[cnt++] = ptr[1];
      image->data[cnt++] = ptr[2];

      if (res) image->data[cnt++] = ptr[3];
    }
  }
  image->sizeX = width;
  image->sizeY = height;

  return res;
}

int TextureManager::loadJPGImage(char *filename, FILE *file, image_t *image)
{
  return -1;
}

int TextureManager::loadBMPImage(char *filename, FILE *file, image_t *image)
{
  unsigned long size;                 // size of the image in bytes.
  unsigned long i;                    // standard counter.
  unsigned short int planes;          // number of planes in image (must be 1) 
  unsigned short int bpp;             // number of bits per pixel (must be 24)
  char temp;                          // temporary color storage for bgr-rgb conversion.

  // seek through the bmp header, up to the width/height:
  fseek(file, 18, SEEK_CUR);

  // read the width
  if ((i = fread(&(image->sizeX), 4, 1, file)) != 1) {
    fprintf(stderr, "Error reading width from %s.\n", filename);
    return -1;
  }
  image->sizeX = (unsigned long)(ABS((signed short)((unsigned short)image->sizeX & 0xffff)));
  Log::getStream(5) << "Width of " << filename << ": " << image->sizeX << endl;
    
  // read the height 
  if ((i = fread(&(image->sizeY), 4, 1, file)) != 1) {
    fprintf(stderr, "Error reading height from %s.\n", filename);
    return -1;
  }
  image->sizeY = (unsigned long)(ABS((signed short)((unsigned short)image->sizeY & 0xffff)));
  Log::getStream(5) << "Height of " << filename << ": " << image->sizeY << endl;
    
  // calculate the size (assuming 24 bits or 3 bytes per pixel).
  size = image->sizeX * image->sizeY * 3;

  // read the planes
  if ((fread(&planes, 2, 1, file)) != 1) {
    fprintf(stderr, "Error reading planes from %s.\n", filename);
    return -1;
  }
  if (planes != 1) {
    fprintf(stderr, "Planes from %s is not 1: %u\n", filename, planes);
    return -1;
  }

  // read the bpp
  if ((i = fread(&bpp, 2, 1, file)) != 1) {
    fprintf(stderr, "Error reading bpp from %s.\n", filename);
    return -1;
  }
  if (bpp != 24) {
    fprintf(stderr, "Bpp from %s is not 24: %u\n", filename, bpp);
    return -1;
  }
	
  // seek past the rest of the bitmap header.
  fseek(file, 24, SEEK_CUR);

  // read the data. 
  image->data = (unsigned char *) malloc(size);
  if (image->data == NULL) {
    fprintf(stderr, "Error allocating memory for color-corrected image data\n");
    return -1;	
  }

  if ((i = fread(image->data, size, 1, file)) != 1) {
    fprintf(stderr, "Error reading image data from %s.\n", filename);
    free(image->data);
    return -1;
  }

  for (i=0; i<size; i+=3) { // reverse all of the colors. (bgr -> rgb)
    temp = image->data[i];
    image->data[i] = image->data[i+2];
    image->data[i+2] = temp;
  }

  return 0;
}

int TextureManager::loadBMPAlphaImage(char *filename, FILE *file, image_t *image)
{
  image_t orig;
  unsigned long i, j;
  unsigned long size;

  if (loadBMPImage(filename, file, &orig) < 0) return -1;

  image->sizeX = orig.sizeX;
  image->sizeY = orig.sizeY;
  image->data = (unsigned char *)malloc(orig.sizeX * orig.sizeY * 4);
  size = orig.sizeX * orig.sizeY * 3;
  for (i = 0, j = 0; i < size; i+=3, j += 4) {
    image->data[j] = orig.data[i];
    image->data[j+1] = orig.data[i+1];
    image->data[j+2] = orig.data[i+2];
    image->data[j+3] = 255;
    if (orig.data[i] == 0 && orig.data[i+1] == 0 && orig.data[i+2] == 0)
      image->data[j+3] = 0;
  }

  free(orig.data);
  return 1;
}

int TextureManager::loadRAWImage(char *filename, FILE *file, unsigned int w, unsigned int h, image_t *image)
{
  if (!file || !image) return -1;

  // allocate buffer
  image->sizeX = 256;
  image->sizeY = 256;
  image->data = (unsigned char *)malloc(w*h*3);
  if (!image->data) {
    printf("Error allocating memory for color-corrected image data\n");
    return -1;
  }

  // read texture data
  unsigned int r = fread(image->data, sizeof(char), w*h*3, file);
  if (r != (unsigned int)(w*h*3)) {
    printf("Error while loading RAW texture from %s\n", filename);
    free(image->data);
    return -1;
  }

  return 0;
}
