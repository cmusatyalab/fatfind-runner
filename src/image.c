#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

double MPI = 3.1415926535897;

typedef struct{
	int rows;
	int cols;
	unsigned char* data;
}sImage;

long getImageInfo(FILE* inputFile, long offset, int numberOfChars){
	unsigned char* ptrC;
	long value = 0L;
	unsigned char dummy = '0';
	int i;
	
	ptrC = &dummy;
	
	fseek(inputFile, offset, SEEK_SET);
	
	for(i = 1; i <= numberOfChars; i++){
		fread(ptrC, sizeof(char), 1, inputFile);
		value = (long)(value + (*ptrC) * (pow(256, (i - 1))));
	}
	
	return(value);
} 

void copyImageInfo(FILE* inputFile, FILE* outputFile){
	unsigned char* ptrC;
	unsigned char dummy = '0';
	int i;
	
	ptrC = &dummy;
	
	fseek(inputFile, 0L, SEEK_SET);
	fseek(outputFile, 0L, SEEK_SET);
	
	for(i = 0; i <= 50; i++){
		fread(ptrC, sizeof(char), 1, inputFile);
		fwrite(ptrC, sizeof(char), 1, outputFile);
	}
}

void copyColorTable(FILE* inputFile, FILE* outputFile, int nColors){
	unsigned char* ptrC;
	unsigned char dummy = '0';
	int i;

	ptrC = &dummy;
	
	fseek(inputFile, 54L, SEEK_SET);
	fseek(outputFile, 54L, SEEK_SET);

	for(i = 0; i <= (4 * nColors); i++){
		fread(ptrC, sizeof(char), 1, inputFile);
		fwrite(ptrC, sizeof(char), 1, outputFile);
	}
}

void CalculateCannyEdgeImage(char* fName){
	FILE *bmpInput, *bmpOutput, *gaussOutput;
	sImage originalImage, gaussImage, edgeImage;
	unsigned int r, c;
	int I, J;
	int sumX, sumY, SUM;
	int leftPixel, rightPixel;
	int P1, P2, P3, P4, P5, P6, P7, P8;
	int nColors;
	unsigned long vectorSize;
	unsigned long fileSize;
	int MASK[5][5];
	int GX[3][3];
	int GY[3][3];
	unsigned char *pChar, someChar;
	unsigned int row, col;
	float ORIENT;
	int edgeDirection;
	int highThreshold, lowThreshold;

	someChar = '0'; 
	pChar = &someChar;

	//...5x5 Gaussian mask.  Ref: http://www.cee.hw.ac.uk/hipr/html/gsmooth.html...
	MASK[0][0] = 2; MASK[0][1] =  4; MASK[0][2] =  5; MASK[0][3] =  4; MASK[0][4] = 2;
	MASK[1][0] = 4; MASK[1][1] =  9; MASK[1][2] = 12; MASK[1][3] =  9; MASK[1][4] = 4;
	MASK[2][0] = 5; MASK[2][1] = 12; MASK[2][2] = 15; MASK[2][3] = 12; MASK[2][4] = 5;
	MASK[3][0] = 4; MASK[3][1] =  9; MASK[3][2] = 12; MASK[3][3] =  9; MASK[3][4] = 4;
	MASK[4][0] = 2; MASK[4][1] =  4; MASK[4][2] =  5; MASK[4][3] =  4; MASK[4][4] = 2;

	//...3x3 GX Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html...
	GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
	GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
	GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;

	//...3x3 GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html...
	GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
	GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
	GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;

	//...Open files for reading and writing to...
	bmpInput = fopen(fName, "rb");
	bmpOutput = fopen("canny.bmp", "wb");
	gaussOutput = fopen("gauss.bmp", "wb");

	//...Start pointer at beginning of file...
	fseek(bmpInput, 0L, SEEK_END);

	//..Retrieve and print file size and number of cols and rows...
	fileSize = getImageInfo(bmpInput, 2, 4);
	originalImage.cols = (int)getImageInfo(bmpInput, 18, 4);
	originalImage.rows = (int)getImageInfo(bmpInput, 22, 4);
	gaussImage.rows = edgeImage.rows = originalImage.rows;
	gaussImage.cols = edgeImage.cols = originalImage.cols;

	//...Retrieve and print Number of colors...
	nColors = (int)getImageInfo(bmpInput, 46, 4);

	//...Allocate memory for originalImage.data...
	vectorSize = fileSize - (14 + 40 + 4 * nColors);
	originalImage.data = (unsigned char*)malloc(vectorSize * sizeof(unsigned char));
	if(originalImage.data == NULL){
		printf("Failed to malloc originalImage.data\n");
		exit(0);
	}

	//...Allocate memory for gaussImage.data...
	//printf("vectorSize: %lu\n", vectorSize);
	gaussImage.data = (unsigned char*)malloc(vectorSize * sizeof(unsigned char));
	if(gaussImage.data == NULL){
		printf("Failed to malloc gaussImage.data\n");
		exit(0);
	}

	//...Allocate memory for edgeImage.data...
	edgeImage.data = (unsigned char*)malloc(vectorSize * sizeof(unsigned char));
	if(edgeImage.data == NULL){
		printf("Failed to malloc edgeImage.data\n");
		exit(0);
	}
	
	copyImageInfo(bmpInput, bmpOutput);
	copyColorTable(bmpInput, bmpOutput, nColors);
	copyImageInfo(bmpInput, gaussOutput);
	copyColorTable(bmpInput, gaussOutput, nColors);

	fseek(bmpInput, (14 + 40 + 4 * nColors), SEEK_SET);
	fseek(bmpOutput, (14 + 40 + 4 * nColors), SEEK_SET);
	fseek(gaussOutput, (14 + 40 + 4 * nColors), SEEK_SET);

	//...Read input.bmp and store it's raster data into originalImage.data... 
	for(row = 0; row <= originalImage.rows - 1; row++){
		for(col = 0; col <= originalImage.cols - 1; col++){
			fread(pChar, sizeof(char), 1, bmpInput);
			*(originalImage.data + row * originalImage.cols + col) = *pChar;
		}
	}

	/**************************************************
	*		 GAUSSIAN MASK ALGORITHM STARTS HERE		 *
	**************************************************/
	for(r = 0; r <= (originalImage.rows - 1); r++){
		for(c = 0; c <= (originalImage.cols - 1); c++){
			SUM = 0;
			
			//...Image boundaries...
			if(r == 0 || r == 1 || r == originalImage.rows - 2 || r == originalImage.rows - 1){
				SUM = *pChar;
			}
			else if(c == 0 || c == 1 || c == originalImage.cols - 2 || c == originalImage.cols - 1){
				SUM = *pChar;
			}  
			//...Convolution starts here...
			else{
				for(I = -2; I <= 2; I++){
					for(J = -2; J <= 2; J++){
						SUM = SUM + (int)((*(originalImage.data + c + I + (r + J) * originalImage.cols)) * (MASK[I + 2][J + 2]) / 115);
					}
				}
			}
			
			if(SUM > 255){
				SUM = 255;
			}

			if(SUM < 0){
				SUM = 0;
			}
		  
			*(gaussImage.data + c + r * originalImage.cols) = (unsigned char)(SUM);
		}
	}


	/*******************************************
	*		 SOBEL GRADIENT APPROXIMATION		 *
	*******************************************/
	for(r = 0; r <= (originalImage.rows - 1); r++){
		for(c = 0; c <= (originalImage.cols - 1); c++){
			sumX = 0;
			sumY = 0;

			//...Image boundaries...
			if(r == 0 || r == originalImage.rows - 1){
				SUM = 0;
			}
			else if(c == 0 || c == originalImage.cols - 1){
				SUM = 0;
			}
			//...Convolution starts here...
			else{
				/***************************************
				*		 X gradient approximation		 *
				***************************************/
				for(I = -1; I <= 1; I++){
					for(J = -1; J <= 1; J++){
						sumX = sumX + (int)((*(gaussImage.data + c + I + (r + J) * originalImage.cols)) * GX[I + 1][J + 1]);
					}
				}

				/***************************************
				*		 Y gradient approximation		 *
				 ***************************************/
				for(I = -1; I <= 1; I++){
					for(J = -1; J <= 1; J++){
						sumY = sumY + (int)((*(gaussImage.data + c + I + (r + J) * originalImage.cols)) * GY[I + 1][J + 1]);
					}
				}
				
				/*************************************************************
				*		 GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)		 *
				*************************************************************/
				SUM = abs(sumX) + abs(sumY);
				
				if(SUM > 255){
					SUM = 255;
				}
				if(SUM < 0){
					SUM = 0;
				}
				
				/************************************
				*		 Magnitude orientation		*
				************************************/
				
				//...Cannot divide by zero...
				if(sumX == 0){
					if(sumY == 0){
						ORIENT = 0.0;
					}
					else if(sumY < 0){
						sumY = -sumY;
						ORIENT = 90.0;
					}
					else ORIENT = 90.0;
				}
				//...Can't take invtan of angle in 2nd Quad...
				else if(sumX < 0 && sumY > 0){
					sumX = -sumX;
					ORIENT = 180 - ((atan((float)(sumY) / (float)(sumX))) * (180 / MPI));
				}
				//...Can't take invtan of angle in 4th Quad...
				else if(sumX > 0 && sumY < 0){
					sumY = -sumY;
					ORIENT = 180 - ((atan((float)(sumY) / (float)(sumX))) * (180 / MPI));
				}
				//..Else angle is in 1st or 3rd Quad...
				else{
					ORIENT = (atan((float)(sumY) / (float)(sumX))) * (180 / MPI);
				}
								
				/***************************************************
				* Find edgeDirection by assigning ORIENT a value of
				* either 0, 45, 90 or 135 degrees, depending on which
				* value ORIENT is closest to
				****************************************************/
			 
				if(ORIENT < 22.5){
					edgeDirection = 0;
				}
				else if(ORIENT < 67.5){
					edgeDirection = 45;
				}
				else if(ORIENT < 112.5){
					edgeDirection = 90;
				}
				else if(ORIENT < 157.5){
					edgeDirection = 135;
				}
				else{
					edgeDirection = 0;
				}

				/***************************************************
				* Obtain values of 2 adjacent pixels in edge
				* direction.
				****************************************************/
			 
				if(edgeDirection == 0){
					leftPixel = (int)(*(gaussImage.data + r * originalImage.cols + c - 1));
					rightPixel = (int)(*(gaussImage.data + r * originalImage.cols + c + 1));
				}
				else if(edgeDirection == 45){
					leftPixel = (int)(*(gaussImage.data + r * originalImage.cols + c + originalImage.cols - 1));
					rightPixel = (int)(*(gaussImage.data + r * originalImage.cols + c - originalImage.cols + 1));
				}
				else if(edgeDirection == 90){
					leftPixel = (int)(*(gaussImage.data + r * originalImage.cols + c - originalImage.cols));
					rightPixel = (int)(*(gaussImage.data + r * originalImage.cols + c + originalImage.cols));
				}
				else{
					leftPixel = (int)(*(gaussImage.data + r * originalImage.cols + c - originalImage.cols - 1));
					rightPixel = (int)(*(gaussImage.data + r * originalImage.cols + c + originalImage.cols + 1));
				}
								
				/*********************************************
				* Compare current magnitude to both adjacent
				* pixel values.  And if it is less than either
				* of the 2 adjacent values - suppress it and make
				* a nonedge.
				*********************************************/
				
				if(SUM < leftPixel || SUM < rightPixel){
					SUM = 0;
				}
				else{
					/**********************
					* Hysteresis
					**********************/
					highThreshold = 120;
					lowThreshold = 40;

					if(SUM >= highThreshold){
						SUM = 255; //...Edge...
					}
					else if(SUM < lowThreshold){
						SUM = 0; //...Nonedge...
					}
					//...SUM is between T1 & T2...
					else{
						//...Determine values of neighboring pixels...
						P1 = (int)(*(gaussImage.data + r * originalImage.cols + c - originalImage.cols - 1));
						P2 = (int)(*(gaussImage.data + r * originalImage.cols + c - originalImage.cols));
						P3 = (int)(*(gaussImage.data + r * originalImage.cols + c - originalImage.cols + 1));
						P4 = (int)(*(gaussImage.data + r * originalImage.cols + c - 1));
						P5 = (int)(*(gaussImage.data + r * originalImage.cols + c + 1));
						P6 = (int)(*(gaussImage.data + r * originalImage.cols + c + originalImage.cols - 1));
						P7 = (int)(*(gaussImage.data + r * originalImage.cols + c + originalImage.cols));
						P8 = (int)(*(gaussImage.data + r * originalImage.cols + c + originalImage.cols + 1));
						
						//...Check to see if neighboring pixel values are edges...
						if(P1 > highThreshold || P2 > highThreshold || P3 > highThreshold || P4 > highThreshold || 
							P5 > highThreshold || P6 > highThreshold || P7 > highThreshold || P8 > highThreshold){
							SUM = 255; //...Make edge...
						}
						else{
							SUM = 0; //...Make nonedge...
						}
					}
				}
			} 
			//...Else loop ends here (starts after b.c.)...
			//fprintf(dataOutput, "New MAG: \t\t%d\n\n\n\n", SUM);
			*(edgeImage.data + c + r * originalImage.cols) = 255 - (unsigned char)(SUM);
			fwrite((edgeImage.data + c + r * originalImage.cols), sizeof(char), 1, bmpOutput);
			fwrite((gaussImage.data + c + r * originalImage.cols), sizeof(char), 1, gaussOutput);
		}
	}

	fclose(bmpInput);
	fclose(bmpOutput);
	fclose(gaussOutput);
	return;
}

void CalculateHoughTransform(char* fName){
	FILE *edgeInput, *lineOutput, *houghOutput;
	sImage edgeImage, lineImage, houghImage;
	int nColors;
	unsigned long vectorSize;
	unsigned long fileSize;
	unsigned char *pChar, someChar;
	unsigned int row, col;

	someChar = '0'; 
	pChar = &someChar;

	//...Open files for reading and writing to...
	edgeInput = fopen(fName, "rb");
	lineOutput = fopen("line.bmp", "wb");
	houghOutput = fopen("hough.bmp", "wb");
	
	//...Start pointer at beginning of file...
	fseek(edgeInput, 0L, SEEK_END);

	//..Retrieve and print file size and number of cols and rows...
	fileSize = getImageInfo(edgeInput, 2, 4);
	edgeImage.cols = (int)getImageInfo(edgeInput, 18, 4);
	edgeImage.rows = (int)getImageInfo(edgeInput, 22, 4);
	houghImage.rows = lineImage.rows = edgeImage.rows;
	houghImage.cols = lineImage.cols = edgeImage.cols;

	//...Retrieve and print Number of colors...
	nColors = (int)getImageInfo(edgeInput, 46, 4);

	//...Allocate memory for originalImage.data...
	vectorSize = fileSize - (14 + 40 + 4 * nColors);
	edgeImage.data = (unsigned char*)malloc(vectorSize * sizeof(unsigned char));
	if(edgeImage.data == NULL){
		printf("Failed to malloc originalImage.data\n");
		exit(0);
	}

	//...Allocate memory for gaussImage.data...
	//printf("vectorSize: %lu\n", vectorSize);
	houghImage.data = (unsigned char*)calloc(vectorSize, sizeof(unsigned char));
	if(houghImage.data == NULL){
		printf("Failed to malloc gaussImage.data\n");
		exit(0);
	}

	//...Allocate memory for edgeImage.data...
	lineImage.data = (unsigned char*)malloc(vectorSize * sizeof(unsigned char));
	if(lineImage.data == NULL){
		printf("Failed to malloc edgeImage.data\n");
		exit(0);
	}
	
	copyImageInfo(edgeInput, lineOutput);
	copyColorTable(edgeInput, lineOutput, nColors);
	copyImageInfo(edgeInput, houghOutput);
	copyColorTable(edgeInput, houghOutput, nColors);

	fseek(edgeInput, (14 + 40 + 4 * nColors), SEEK_SET);
	fseek(lineOutput, (14 + 40 + 4 * nColors), SEEK_SET);
	fseek(houghOutput, (14 + 40 + 4 * nColors), SEEK_SET);

	//...Read input.bmp and store it's raster data into originalImage.data... 
	for(row = 0; row <= edgeImage.rows - 1; row++){
		for(col = 0; col <= edgeImage.cols - 1; col++){
			fread(pChar, sizeof(char), 1, edgeInput);
			*(edgeImage.data + row * edgeImage.cols + col) = *pChar;
		}
	}

	//...Create the hough transform of the image...
	int max_radius = (int)sqrt(edgeImage.rows * edgeImage.rows + edgeImage.cols * edgeImage.cols);
	
	for(row = 0; row <= edgeImage.rows - 1; row++){
		for(col = 0; col <= edgeImage.cols - 1; col++){			
			if(*(edgeImage.data + row * edgeImage.cols + col) > 0){
				for(int t = 0; t < 360; t++){
					int r = (int)((row - edgeImage.rows / 2) * cos(MPI / 180.0 * t) + (col - edgeImage.cols / 2) * sin(MPI / 180.0 * t));
					if((r > -max_radius / 2) && (r < max_radius / 2) && (r != 0)){
						*(houghImage.data + t + (r + max_radius / 2) * houghImage.cols) += 1;
					}
				}
			}
		}
	}

	//...Detect lines and draw them on the line image...
	int h_width = 360;
	int h_height = (int)max_radius;

	int max_accum = 0;
	int dr, dt;
	int maxima, max_value = 0;

	//...The the maxima in the accumulator...
	for(row = 0; row < h_height; row++){
		for(col = 0; col < h_width; col++){
			if(*(houghImage.data + row * houghImage.cols + col) > max_value){
				max_value = *(houghImage.data + row * houghImage.cols + col);
			}
		}
	}

	//...Detect local maximas...
	for(row = 4; row < h_height - 5; row++){
		for (col = 4; col < h_width - 5; col++){
			if(*(houghImage.data + row * houghImage.cols + col) > 0.7 * max_value){
				maxima = 1;

				for(dr = row-4; dr < row+5; dr++){
					for(dt = col-5; dt < col+5; dt++){
						if(*(houghImage.data + dr * houghImage.cols + dt) > *(houghImage.data + row * houghImage.cols + col)){
							maxima = 0;
						}
					}
				}

				if(maxima == 1){
					//...Draw the line...
					float radius = (float)(row - h_height / 2);
					float theta = (float)((col-180)*MPI/180.0);
					float M, B;
					int x, y;
					int start, end;

					if(((theta >= MPI/4) && (theta <= 3*MPI/4)) || ((-theta >= MPI/4) && (-theta <= 3*MPI/4))){
						M = (float)cos(theta) / (float)sin(theta);
						B = radius / (float)sin(theta) + (houghImage.cols/2) * M + houghImage.rows / 2;
						start = end = -1;
						for(x = 0; x < houghImage.cols; x++){
							y = (int)(0.5 + B - x * M);
							if((y >= 0) && (y < houghImage.rows ))
								if (start < 0)
								start = x;
							else
							{
								end = x;
								for (x = start; x < end; x++)
								{
									y = (int) (0.5 + B - x * M);
									*(edgeImage.data + y * edgeImage.cols + x) = 150;
								}
								start = end = -1;
							}		
						}
					}
					else{
						M = (float)sin(theta) / (float)cos(theta);
						B = radius / (float)cos(theta) + (houghImage.rows /2) * M + houghImage.cols /2;
						start = end = -1;
						for(y = 0; y < houghImage.rows; y++){
							x = (int)(0.5 + B - y * M);
							if((x >= 0) && (x < houghImage.cols)){
								if(start < 0){
									start = y;
								}
								else{
									end = y;
									for(y = start; y < end; y++){
										x = (int)(0.5 + B - y * M);
										*(edgeImage.data + y * edgeImage.cols + x) = 150;	
									}
									start = end = -1;
								}
							}
						}
					}
				}
			}
		}
	}

	//...Write the output to files...
	for(row = 0; row <= edgeImage.rows - 1; row++){
		for(col = 0; col <= edgeImage.cols - 1; col++){
			*(houghImage.data + col + row * houghImage.cols) *= 200;
			*(lineImage.data + col + row * lineImage.cols) = *(edgeImage.data + row * edgeImage.cols + col);
			fwrite((houghImage.data + col + row * houghImage.cols), sizeof(char), 1, houghOutput);
			fwrite((lineImage.data + col + row * lineImage.cols), sizeof(char), 1, lineOutput);
		}
	}

	fclose(edgeInput);
	fclose(lineOutput);
	fclose(houghOutput);
	
	return;
}

int applyTransforms(int argc, char* argv[]){

	if(argc < 2){
		printf("Usage: %s bmpInput.bmp\n", argv[0]);
		exit(0);
	};
	

	printf("Applying Edge Detector...\n");
	CalculateCannyEdgeImage(argv[1]);
	printf("Applying Circle Hough Transform...\n");
	CalculateHoughTransform("canny.bmp");

	return 0;
}
