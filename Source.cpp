#include <stdlib.h>
#include <GL/freeglut.h>
#include <FreeImage.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <iostream>
#include <cstring>
#include <math.h>

using namespace std;

#define max(x,y) (((x) > (y)) ? (x) : (y))
#define min(x,y) (((x) < (y)) ? (x) : (y))

/**
* Data structure for Pixel type
* stores pixel color data in the form
* of GLubyte
*/
typedef struct {
	// pixel channels for RGB
	GLubyte red, green, blue;
} Pixel;

/**
* Data structure for Image type
* a collection of Pixels to store the image upon
*/
typedef struct {
	Pixel *data; // collection of pixels
	int width, height; // dimensions
} Image;

// some function declarations
int checkCloser(int col[3], int rgbVals[9][3]);
void changeConvolution(Image &img, char type);
Image copyImage(Image);

// global work and save buffers (easier than local scope)
Image workBuffer, saveBuffer;

/**
* Image Loader function
* loads an input image into memory
*
* @param name - the filename of the loaded file
* @return - image save buffer
*/
Image imageLoader(char *name) {
	FIBITMAP *inputImage; // container for input image
	inputImage = FreeImage_Load(FIF_TIFF, name, 0); //attempts to load
	Image outputImage; // for returning later
	// set up image dimensions
	outputImage.width = FreeImage_GetWidth(inputImage);
	outputImage.height = FreeImage_GetHeight(inputImage);
	RGBQUAD pixelData; // container for image pixel data
	Pixel *data; // blank pixel structure
	 // ensure correct size
	data = (Pixel*)malloc((outputImage.height)*(outputImage.width) * sizeof(Pixel));
	int k = 0; // for iterating in a moment
	for (int i = 0; i < outputImage.height; i++) {
		for (int j = 0; j < outputImage.width; j++, k++) {
			// mutate pixelData to contain RGB values of given pixel
			FreeImage_GetPixelColor(inputImage, j, i, &pixelData);
			// initialize color channels of pixel to match
			data[k].red = pixelData.rgbRed;
			data[k].green = pixelData.rgbGreen;
			data[k].blue = pixelData.rgbBlue;
		}
	}
	FreeImage_Unload(inputImage); // no longer needed in memory
	outputImage.data = data; // initialize image pixels
	return outputImage; // to instantiate image in main driver
}

/**
* Save Image function
* effectively the load image function in reverse
*
* @param name - filename to save as
* @param i - the image to save
*/
void saveImage(char *name, Image img) {
	FIBITMAP *outputImage; // output image container
	// allocate memory to output image container
	outputImage = FreeImage_Allocate(img.width, img.height, 24, 0, 0, 0);
	/**
	 * refer to load image function to get the gist
	 * as this is just the same code more or less
	 */
	RGBQUAD pixelData;
	int k = 0;
	for (int i = 0; i < img.height; i++) {
		for (int j = 0; j < img.width; j++, k++) {
			pixelData.rgbRed = img.data[k].red;
			pixelData.rgbGreen = img.data[k].green;
			pixelData.rgbBlue = img.data[k].blue;
			FreeImage_SetPixelColor(outputImage, j, i, &pixelData);
		}
	}
	FreeImage_Save(FIF_TIFF, outputImage, name, 0);
	FreeImage_Unload(outputImage);
}

/**
 * Greyscale Filter function
 * applies one of two GS filters to image
 *
 * @param img - the image to work with
 * @param type - the type of GS algorithm to use
 */
void changeGrey(Image &img, char type) {
	int k = 0;
	int lum = 0; // luminance
	double rMult, gMult, bMult; // multipliers
	if (type == 'N') { // NTSC greyscale
		rMult = 0.30;
		gMult = 0.59;
		bMult = 0.11;
	}
	else { // regular greyscale
		rMult = 0.33;
		gMult = 0.33;
		bMult = 0.33;
	}
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++, lum = 0) {
			lum += img.data[k].red*rMult; // add to luminance
			lum += img.data[k].green*gMult;
			lum += img.data[k].blue*bMult;
			img.data[k].red = lum; // apply
			img.data[k].green = lum;
			img.data[k].blue = lum;
		}
	}
}

/**
 * Monochrome Filter function
 * changes image to B+W
 *
 * @param img - the image to binarize
 */
void changeMonochrome(Image &img) {
	int k = 0;
	int lum = 0;
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++, lum = 0) {
			lum += img.data[k].red*0.33;
			lum += img.data[k].green*0.33;
			lum += img.data[k].blue*0.33;
			/** if higher than half, set to black
			 * if lower, set to white, etc
			 */
			img.data[k].red = (lum > 128) ? 255 : 0;
			img.data[k].green = (lum > 128) ? 255 : 0;
			img.data[k].blue = (lum > 128) ? 255 : 0;
		}
	}
}

/**
 * Channel Swap Filter function
 * swaps R->G, G->B, B->R
 *
 * @param img - the image to work with
 */
void changeSwap(Image &img) {
	int k = 0;
	int temp = 0;
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			// self explanatory
			temp = img.data[k].red;
			img.data[k].red = img.data[k].green;
			img.data[k].green = img.data[k].blue;
			img.data[k].blue = temp;
		}
	}
}

/**
 * Single Channel Filter function
 * changes image color channels to singular
 *
 * @param img - the image to work with
 * @param type - the type of channel to filter
 */
void changeSingleChannel(Image &img, char type) {
	int k = 0;
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			if (type == 'R') { // if red
				img.data[k].green = 0; // set G = 0
				img.data[k].blue = 0; // B = 0
			}
			else if (type == 'G') { // if green
				img.data[k].red = 0;
				img.data[k].blue = 0;
			}
			else { // else blue
				img.data[k].red = 0;
				img.data[k].green = 0;
			}
		}
	}
}

/**
 * Maximize Filter function
 * replaces RGB channels with maximum channel intensity
 * of surrounding 9 pixels
 *
 * @param img - the image to work with
 */
void changeMax(Image &img) {
	Image temp = copyImage(img); // to not clobber, etc
	int k = 0;
	int rgbTemp[3] = { 0, 0, 0 };
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			rgbTemp[0] = temp.data[k].red;
			rgbTemp[1] = temp.data[k].green;
			rgbTemp[2] = temp.data[k].blue;
			/**
			 * this is fairly self explanatory:
			 * replace each pixel RGB channel with maximum
			 * channel intensity of adjacent pixels
			 */
			if (k - img.width - 1 >= 0) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k - img.width - 1].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k - img.width - 1].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k - img.width - 1].blue);
			}
			if (k - img.width >= 0) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k - img.width].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k - img.width].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k - img.width].blue);
			}
			if (k - img.width + 1 >= 0) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k - img.width + 1].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k - img.width + 1].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k - img.width + 1].blue);
			}
			if ((k - 1) % img.width != 0) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k - 1].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k - 1].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k - 1].blue);
			}
			if ((k + 1) % (img.width) != 0) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k + 1].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k + 1].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k + 1].blue);
			}
			if (k + img.width - 1 < img.height*img.width) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k + img.width - 1].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k + img.width - 1].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k + img.width - 1].blue);
			}
			if (k + img.width < img.height*img.width) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k + img.width].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k + img.width].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k + img.width].blue);
			}
			if (k + img.width + 1 < img.height*img.width) {
				rgbTemp[0] = max(rgbTemp[0], temp.data[k + img.width + 1].red);
				rgbTemp[1] = max(rgbTemp[1], temp.data[k + img.width + 1].green);
				rgbTemp[2] = max(rgbTemp[2], temp.data[k + img.width + 1].blue);
			}
			img.data[k].red = rgbTemp[0];
			img.data[k].green = rgbTemp[1];
			img.data[k].blue = rgbTemp[2];
		}
	}
}

/**
 * Minimize Filter function
 * see Max Filter function, it's the exact same
 * but min() instead of max() used
 *
 * @param img - the image to work with
 */
void changeMin(Image &img) {
	Image temp = copyImage(img);
	int k = 0;
	int rgbTemp[3] = { 0, 0, 0 };
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			rgbTemp[0] = temp.data[k].red;
			rgbTemp[1] = temp.data[k].green;
			rgbTemp[2] = temp.data[k].blue;
			if (k - img.width - 1 >= 0) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k - img.width - 1].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k - img.width - 1].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k - img.width - 1].blue);
			}
			if (k - img.width >= 0) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k - img.width].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k - img.width].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k - img.width].blue);
			}
			if (k - img.width + 1 >= 0) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k - img.width + 1].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k - img.width + 1].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k - img.width + 1].blue);
			}
			if ((k - 1) % img.width != 0) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k - 1].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k - 1].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k - 1].blue);
			}
			if ((k + 1) % (img.width) != 0) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k + 1].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k + 1].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k + 1].blue);
			}
			if (k + img.width - 1 < img.height*img.width) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k + img.width - 1].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k + img.width - 1].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k + img.width - 1].blue);
			}
			if (k + img.width < img.height*img.width) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k + img.width].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k + img.width].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k + img.width].blue);
			}
			if (k + img.width + 1 < img.height*img.width) {
				rgbTemp[0] = min(rgbTemp[0], temp.data[k + img.width + 1].red);
				rgbTemp[1] = min(rgbTemp[1], temp.data[k + img.width + 1].green);
				rgbTemp[2] = min(rgbTemp[2], temp.data[k + img.width + 1].blue);
			}
			img.data[k].red = rgbTemp[0];
			img.data[k].green = rgbTemp[1];
			img.data[k].blue = rgbTemp[2];
		}
	}
}

/**
 * Intensity Filter function
 * increase intensity of certain color channels
 *
 * @param img - the image to work with
 * @param type - the color channel to intensify
 */
void changeIntensity(Image &img, char type) {
	int k = 0;
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			/**
			 * depending on type, change color channel
			 * intensity by 15% each time the
			 * algorithm is run
			 */
			if (type == 'R') {
				img.data[k].red = min(255, img.data[k].red*1.15);
			}
			else if (type == 'G') {
				img.data[k].green = min(255, img.data[k].green * 1.15);
			}
			else {
				img.data[k].blue = min(255, img.data[k].blue * 1.15);
			}
		}
	}
}

/**
 * Edge Detection function
 * performs both Sobel operations on
 * the image, with a color variant as bonus
 *
 * @param img - the image to work with
 * @param colorToggle - whether or not you need color
 */
void bothEdges(Image &img, char colorToggle) {
	if (colorToggle != 'C') { // if color edges
		changeGrey(img, 'G');
	} // otherwise greyscale
	changeConvolution(img, 'H');
	changeConvolution(img, 'V');
}

/**
 * Convolution Filter function
 * applies a specific kernel to the image
 *
 * @param img - the image to work with
 * @param type - the type of kernel to use
 */
void changeConvolution(Image &img, char type) {
	// the kernel as represented as 2D array
	static int matrices[5][9] = {
		{ 1, 2, 1, 0, 0, 0, -1, -2, -1 }, // Sobel H
		{ 1, 0, -1, 2, 0, -2, 1, 0, -1 }, // Sobel V
		{ 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Blur
		{ 1, 2, 1, 2, 4, 2, 1, 2, 1 }, // Gauss. Blur
		{ 0, -1, 0, -1, 5, -1, 0, -1, 0 } // Sharpen
	};
	int *matrix; // get subarray from matrix
	if (type == 'H') { // Sobel Horizontal
		matrix = matrices[0];
	}
	else if (type == 'V') { // Sobel Vertical
		matrix = matrices[1];
	}
	else if (type == 'G') { // Gaussian Blur
		matrix = matrices[3];
	}
	else if (type == 'S') { // Sharpen
		matrix = matrices[4];
	}
	else { // Regular Blur
		matrix = matrices[2];
	}
	// init a temporary image to work with
	Image tempImg = copyImage(img);
	int k = 0; // for iterating in a moment
	int l = 0; // to divide later
	int rgbTemp[3] = { 0, 0, 0 };
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++, l = 0) {
			/**
			 * this is pretty ugly, but it applies the kernel
			 * operation to a pixel and adjacent pixels
			 */
			l += matrix[4]; // initial position k needs to be counted, too
			// init initial rgbTemp values to be altered later
			rgbTemp[0] += tempImg.data[k].red*matrix[4];
			rgbTemp[1] += tempImg.data[k].green*matrix[4];
			rgbTemp[2] += tempImg.data[k].blue*matrix[4];
			if (k - img.width - 1 >= 0) { // top-left
				l += matrix[0];
				rgbTemp[0] += tempImg.data[k - img.width - 1].red*matrix[0];
				rgbTemp[1] += tempImg.data[k - img.width - 1].green*matrix[0];
				rgbTemp[2] += tempImg.data[k - img.width - 1].blue*matrix[0];
			} 
			if (k - img.width >= 0) { // above
				l += matrix[1];
				rgbTemp[0] += tempImg.data[k - img.width].red*matrix[1];
				rgbTemp[1] += tempImg.data[k - img.width].green*matrix[1];
				rgbTemp[2] += tempImg.data[k - img.width].blue*matrix[1];
			}
			if (k - img.width + 1 >= 0) { // top-right
				l += matrix[2];
				rgbTemp[0] += tempImg.data[k - img.width + 1].red*matrix[2];
				rgbTemp[1] += tempImg.data[k - img.width + 1].green*matrix[2];
				rgbTemp[2] += tempImg.data[k - img.width + 1].blue*matrix[2];
			} 
			if ((k - 1) % img.width != 0) { // left
				l += matrix[3];
				rgbTemp[0] += tempImg.data[k - 1].red*matrix[3];
				rgbTemp[1] += tempImg.data[k - 1].green*matrix[3];
				rgbTemp[2] += tempImg.data[k - 1].blue*matrix[3];
			} 
			if ((k + 1) % (img.width) != 0) { // right
				l += matrix[5];
				rgbTemp[0] += tempImg.data[k + 1].red*matrix[5];
				rgbTemp[1] += tempImg.data[k + 1].green*matrix[5];
				rgbTemp[2] += tempImg.data[k + 1].blue*matrix[5];
			} 
			if (k + img.width - 1 < img.height*img.width) { // bottom-left
				l += matrix[6];
				rgbTemp[0] += tempImg.data[k + img.width - 1].red*matrix[6];
				rgbTemp[1] += tempImg.data[k + img.width - 1].green*matrix[6];
				rgbTemp[2] += tempImg.data[k + img.width - 1].blue*matrix[6];
			} 
			if (k + img.width < img.height*img.width) { // below
				l += matrix[7];
				rgbTemp[0] += tempImg.data[k + img.width].red*matrix[7];
				rgbTemp[1] += tempImg.data[k + img.width].green*matrix[7];
				rgbTemp[2] += tempImg.data[k + img.width].blue*matrix[7];
			} 
			if (k + img.width + 1 < img.height*img.width) { // bottom-right
				l += matrix[8];
				rgbTemp[0] += tempImg.data[k + img.width + 1].red*matrix[8];
				rgbTemp[1] += tempImg.data[k + img.width + 1].green*matrix[8];
				rgbTemp[2] += tempImg.data[k + img.width + 1].blue*matrix[8];
			}
			/**
			 * we need to clamp down the bounds so you don't get
			 * below or above RGB range. Additionally, clamp the divisor
			 * for edge detection so we don't get division by zero
			 */
			img.data[k].red = max(0, min(255, rgbTemp[0] / max(l,1)));
			img.data[k].green = max(0, min(255, rgbTemp[1] / max(l,1)));
			img.data[k].blue = max(0, min(255, rgbTemp[2] / max(l,1)));
			memset(rgbTemp, 0, sizeof(rgbTemp)); // reset this
		}
	}
}

/**
 * Quantizer Filter function
 * apply a quantized filter to image
 *
 * @param img - the image to work with
 * @param type - the type of filter
 */
void changeQuantize(Image &img, char type) {
	srand(time(NULL)); // seed for random
	int k = 0; // kth pixel
	int highestIndex = 0; // the index of the highest value
	int col[3] = { 0, 0, 0 }; // placeholder to put temp color values
	int vals[9][3] = {
		{ 255, 0, 0 },{ 0, 255, 0 },{ 0, 0, 255 }, // red, green, blue
		{ 255, 255, 255 },{ 0, 0, 0 },{ 128, 128, 128 }, // black, white, grey
		{ 128, 128, 0 },{ 0, 128, 128 },{ 128, 0, 128 } // combos of RGB
	};
	if (type == 'R') { // if random modifier
		for (int i = 0; i < 9; i++) { // 9 colors
			for (int j = 0; j < 3; j++) { // 3 channels per color
				vals[i][j] = rand() % 255; // random value 0-255 per channel
			}
		}
	}
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			col[0] = img.data[k].red;
			col[1] = img.data[k].green;
			col[2] = img.data[k].blue;
			// find index of closest color
			highestIndex = checkCloser(col, vals);
			// replace channel with highest of palette
			img.data[k].red = vals[highestIndex][0];
			img.data[k].green = vals[highestIndex][1];
			img.data[k].blue = vals[highestIndex][2];
		}
	}
}

/**
 * Closest Neighbor function
 * finds closer pixel based on Euclidean geometry in RGB-space
 *
 * @param col - 
 * @param rgbVals -
 * @return - the closest index
 */
int checkCloser(int col[3], int rgbVals[9][3]) {
	int highMatch[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int r, g, b;
	int euDist;
	float squareA, squareB;
	int tempHighest = 0; // temporary highest
	int highestIndex = 0; // index of highest
	for (int i = 0; i < 9; i++) {
		r = rgbVals[i][0];
		g = rgbVals[i][1];
		b = rgbVals[i][2];
		// distance in RGB-space
		squareA = (col[0] - r) * (col[0] - r);
		squareB = (col[2] - b) * (col[2] - b);
		euDist = (int)sqrt(squareA + squareB);
		highMatch[i] = euDist; // insert potential highest values into array
	}
	// finds the highest value and the index of the highest value
	for (int i = 0; i < 9; i++) {
		if (tempHighest < highMatch[i]) {
			tempHighest = highMatch[i];
			highestIndex = i;
		}
	}
	return highestIndex; // return the index of the closest match
}

/**
 * Negative Filter function
 * changes image to color negative
 *
 * @param img - the image to negate
 */
void changeNegative(Image &img) {
	int k = 0;
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			// basically just set to |RGB-255|
			img.data[k].red = abs(img.data[k].red - 255);
			img.data[k].green = abs(img.data[k].green - 255);
			img.data[k].blue = abs(img.data[k].blue - 255);
		}
	}
}

/**
 * Sepia Filter function
 * applies a sepia filter to the image
 *
 * @param img - the image to work with
 */
void changeSepia(Image &img) {
	int k = 0;
	for (int i = 0; i < img.width; i++) {
		for (int j = 0; j < img.height; j++, k++) {
			// values taken from online, standard sepia values
			img.data[k].red = min(255, 
				(img.data[k].red*0.393) + 
				(img.data[k].green*0.769) + 
				(img.data[k].blue*0.189));
			img.data[k].green = min(255, 
				(img.data[k].red*0.349) + 
				(img.data[k].green*0.686) + 
				(img.data[k].blue*0.168));
			img.data[k].blue = min(255, 
				(img.data[k].red*0.272) + 
				(img.data[k].green*0.534) + 
				(img.data[k].blue*0.131));
		}
	}
}

/**
* Display Image function
* taken from template solution
*/
void displayImage(void) {
	glDrawPixels(workBuffer.width, workBuffer.height, GL_RGB, GL_UNSIGNED_BYTE, (GLubyte*)workBuffer.data);
	glFlush();
}

/**
* Copy Image function
* make a copy from Image A to Image B
*
* @param i - the image to copy
* @return - a copy of that image
*/
Image copyImage(Image img) {
	Image tempImage; // create temporary image
	// copy dimensions over
	tempImage.height = img.height;
	tempImage.width = img.width;
	// allocate memory for next pixel data
	tempImage.data = (Pixel*)malloc(sizeof(Pixel)*img.width*img.height);
	// copy memory from i to return image
	memcpy(tempImage.data, img.data, sizeof(Pixel)*img.width*img.height);
	return tempImage; // return the copy
}

/**
 * Menu Display function
 * displays CLI menu to user
 */
void printMenu() {
	cout << "Q: Quit\tR: Reset\t S: Save" << endl;
	cout << "\nDisplay" << endl;
	cout << "1: Greyscale\t2: NTSC\t3: Monochrome\t4: Channel Swap" << endl;
	cout << "5: Pure R\t6: Pure G\t7: Pure B" << endl;
	cout << "\nBasic Channel Filters" << endl;
	cout << "8: Max\t9: Min\t0: Int R\ta: Int G\tb: Int B" << endl;
	cout << "\nConvolution Filters" << endl;
	cout << "c: GS Edges\td: Color Edges" << endl;
	cout << "e: Blur\tf: Gauss. Blur\tg: Sharpen" << endl;
	cout << "\nQuantize Filters" << endl;
	cout << "h: Fixed RGB\ti: Random RGB" << endl;
	cout << "\nCustom Filters" << endl;
	cout << "j: Image Negative\tk: Sepia Filter" << endl;
}
/**
 * Menu Handler function
 * Glut needs argument for menu to perform operations
 * on image. The CLI menu serves as instruction for this
 * switch statement
 */
void menu(unsigned char key, int x, int y) {
	switch (key) {
	case 'q': { exit(0); break; }
	case 'r': { workBuffer = copyImage(saveBuffer);	glutPostRedisplay(); break; }
	case 's': {	saveImage("backup.tif", workBuffer); break; }
	case '1': { changeGrey(workBuffer, 'G'); glutPostRedisplay(); break; }
	case '2': {	changeGrey(workBuffer, 'N'); glutPostRedisplay(); break; }
	case '3': { changeMonochrome(workBuffer); glutPostRedisplay(); break; }
	case '4': { changeSwap(workBuffer);	glutPostRedisplay(); break; }
	case '5': { changeSingleChannel(workBuffer, 'R'); glutPostRedisplay(); break; }
	case '6': { changeSingleChannel(workBuffer, 'G'); glutPostRedisplay(); break; }
	case '7': { changeSingleChannel(workBuffer, 'B'); glutPostRedisplay(); break; }
	case '8': { changeMax(workBuffer); glutPostRedisplay(); break; }
	case '9': { changeMin(workBuffer); glutPostRedisplay();	break; }
	case '0': { changeIntensity(workBuffer, 'R'); glutPostRedisplay(); break; }
	case 'a': { changeIntensity(workBuffer, 'G'); glutPostRedisplay(); break; }
	case 'b': { changeIntensity(workBuffer, 'B'); glutPostRedisplay(); break; }
	case 'c': { bothEdges(workBuffer, 'C'); glutPostRedisplay(); break;	}
	case 'd': { bothEdges(workBuffer, 'N'); glutPostRedisplay(); break;	}
	case 'e': { changeConvolution(workBuffer, 'C'); glutPostRedisplay(); break;	}
	case 'f': { changeConvolution(workBuffer, 'G'); glutPostRedisplay(); break; }
	case 'g': { changeConvolution(workBuffer, 'S');	glutPostRedisplay(); break; }
	case 'h': { changeQuantize(workBuffer, 'F'); glutPostRedisplay();	break; }
	case 'i': { changeQuantize(workBuffer, 'R'); glutPostRedisplay();	break; }
	case 'j': { changeNegative(workBuffer); glutPostRedisplay(); break; }
	case 'k': { changeSepia(workBuffer); glutPostRedisplay(); break; }
	}
}

int main(int argc, char** argv) {
	saveBuffer = imageLoader(argv[1]); // load save buffer
	workBuffer = copyImage(saveBuffer); // create work buffer
	printMenu(); // print CLI interface
	// Glut stuff
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
	glutInitWindowSize(workBuffer.width, workBuffer.height);
	glutCreateWindow("3P98 Assignment 1");
	glutKeyboardFunc(menu);
	glutDisplayFunc(displayImage);
	glutMainLoop();
	return 0;
}
