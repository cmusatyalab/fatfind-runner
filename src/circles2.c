#include <cv.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "circles2.h"
#include "lib_filter.h"


GList *circles;  // XXX for gui only, not filter

static const int accumulatorMax = 1000;

typedef struct {
  int dp;
  int minDist;
  int blur;

  int accumulatorThresh;

  int minRadius;
  int maxRadius;
  int radiusStep;

  int cannyThreshold;

  double scale;

  IplImage *scaledImage;
  IplImage *gray;
} circles_state_t;


// static parameters
static circles_state_t staticState =
  {1, 0, 2, 400, 5, 10, 2, 100, 1.0, NULL, NULL};


static void addToMatrix (int *data, int x, int y, int rows, int cols) {
  if (!(x < 0 || y < 0 || x >= rows || y >= cols)) {
    data[x * cols + y]++;
  }
}

static void circlePoints(CvMat *img, int cx, int cy, int x, int y)
{
  int *data = img->data.i;
  int rows = img->rows;
  int cols = img->cols;
  if (x == 0) {
    addToMatrix(data, cx, cy + y, rows, cols);
    addToMatrix(data, cx, cy - y, rows, cols);
    addToMatrix(data, cx + y, cy, rows, cols);
    addToMatrix(data, cx - y, cy, rows, cols);
  } else if (x == y) {
    addToMatrix(data, cx + x, cy + y, rows, cols);
    addToMatrix(data, cx - x, cy + y, rows, cols);
    addToMatrix(data, cx + x, cy - y, rows, cols);
    addToMatrix(data, cx - x, cy - y, rows, cols);
  } else if (x < y) {
    addToMatrix(data, cx + x, cy + y, rows, cols);
    addToMatrix(data, cx - x, cy + y, rows, cols);
    addToMatrix(data, cx + x, cy - y, rows, cols);
    addToMatrix(data, cx - x, cy - y, rows, cols);
    addToMatrix(data, cx + y, cy + x, rows, cols);
    addToMatrix(data, cx - y, cy + x, rows, cols);
    addToMatrix(data, cx + y, cy - x, rows, cols);
    addToMatrix(data, cx - y, cy - x, rows, cols);
  }
}

static void accumulateCircle(circles_state_t *cr,
			     CvMat *img, int xCenter, int yCenter, int radius)
{
  int x = 0;
  int y = radius;
  int p = (5 - radius*4)/4;


  //printf("radius: %d, addAmount: %g\n", radius, addAmount);

  circlePoints(img, xCenter, yCenter, x, y);
  while (x < y) {
    x++;
    if (p < 0) {
      p += 2*x+1;
    } else {
      y--;
      p += 2*(x-y)+1;
    }
    circlePoints(img, xCenter, yCenter, x, y);
  }
}

static GList *myHoughCircles (circles_state_t *cr,
			      IplImage *image, GList *clist) {
  const int numSlices = (cr->maxRadius - cr->minRadius) / cr->radiusStep;

  CvMat **acc;
  int i, j, x, y;

  const int w = image->width / cr->dp;
  const int h = image->height / cr->dp;

  IplImage *houghScaledImage = cvCreateImage(cvSize(w, h), 8, 1);
  cvResize(image, houghScaledImage, CV_INTER_NN);


  // create accumulator stack
  acc = (CvMat**) cvAlloc(sizeof(CvMat*) * numSlices);
  //  printf("allocating slices ");
  //fflush(stdout);
  for (i = 0; i < numSlices; i++) {
    acc[i] = cvCreateMat(w, h, CV_32SC1);
    //printf(" %4d", i);
    //fflush(stdout);

    cvZero(acc[i]);
    //printf("z");
    //fflush(stdout);
  }

  // do canny
  cvSmooth(image, image, CV_GAUSSIAN, cr->blur*2+1, cr->blur*2+1, 0);
  //  cvCanny(image, image, MAX(cannyThreshold/4,1), cannyThreshold, 3);
  cvCanny(image, image, 0, cr->cannyThreshold, 3);
  //  cvResize(houghScaledImage, image, CV_INTER_NN);

  // for each pixel in the canny, draw into the accumulator for each radius
  for (y = h - 1; y >= 0; y--) {
    printf(".");
    fflush(stdout);
    for (x = w - 1; x >= 0; x--) {
      // is there an edge at this point?
      if (image->imageData[y * cr->dp * image->widthStep + x * cr->dp]) {
      //      if (cvGet2D(image, y * dp, x * dp).val[0]) {
	for (i = 0; i < numSlices; i++) {
	  CvMat *slice = acc[i];
	  int radius = cr->minRadius + (cr->radiusStep * i);

	  //printf("drawing circle: (%d,%d,%d)\n", x, y, radius);
	  accumulateCircle(cr, slice, x, y, radius);
	}
      }
    }
  }
  printf("\n");


  // walk over accumulator, eliminate anything < threshold
  for (i = 0; i < numSlices; i++) {
    CvMat *slice = acc[i];
    int radius = cr->minRadius + (cr->radiusStep * i);
    double circ = 2.0 * M_PI * radius;
    double thresh = circ * ((double) cr->accumulatorThresh) /
      ((double) accumulatorMax);
    int nonZero;

    nonZero = cvCountNonZero(slice);
    printf("nonZero before thresh: %d\n", nonZero);
    for (y = h - 1; y >= 0; y--) {
      for (x = w - 1; x >= 0; x--) {
	/*
	if (cvGet2D(slice, x, y).val[0] < thresh) {
	  cvSet2D(slice, x, y, cvScalarAll(0.0));
	}
	*/
	if (slice->data.i[y * slice->rows + x] < thresh) {
	  slice->data.i[y * slice->rows + x] = 0.0;
	}
      }
    }
    //cvThreshold(slice, slice, thresh, 0, CV_THRESH_TOZERO);
    nonZero = cvCountNonZero(slice);
    printf("slice: %d, radius: %d, thresh: %g, nonZero: %d\n", i, radius, thresh, nonZero);
  }


  // walk over accumulator again, searching for maxima and wiping out regions around them
  for (i = 0; i < numSlices; i++) {
    //  for (i = numSlices - 1; i >= 0; i--) {
    CvMat *slice = acc[i];
    double min_val, max_val;
    CvPoint max_loc;
    int radius = cr->minRadius + (cr->radiusStep * i);

    cvMinMaxLoc(slice, &min_val, &max_val, NULL, &max_loc, NULL);
    while(max_val > 0) {
      // found a point
      circle_type *c= g_malloc(sizeof(circle_type));
      int x = max_loc.y;
      int y = max_loc.x;

      //      printf("circle found; r: %d\n", radius);

      c->x = x * cr->dp / cr->scale;
      c->y = y * cr->dp / cr->scale;
      c->r = radius * cr->dp / cr->scale;
      clist = g_list_prepend(clist, c);

      // wipe out region in all slices
      for (j = i; j < numSlices; j++) {
	//for (j = 0; j <= i; j++) {
	int newRadius = cr->minRadius + (cr->radiusStep * j);
	cvCircle(acc[j], max_loc, cr->minDist + newRadius,
		 cvScalarAll(0), -1, 8, 0);
      }

      // find again
      cvMinMaxLoc(slice, &min_val, &max_val, NULL, &max_loc, NULL);
    }
  }


  // free
  for (i = 0; i < numSlices; i++) {
    cvReleaseMat(&(acc[i]));
  }
  cvFree((void *) &acc);

  return clist;
}

static GList *computeCircles(circles_state_t *cr,
			     int pos, IplImage *initialImage, GList *clist) {
  IplImage* tmpGrayImage = cvCreateImage(cvGetSize(cr->scaledImage), 8, 3);
  IplImage* scaledGray = cvCreateImage(cvGetSize(cr->scaledImage), 8, 1);

  // get a fresh copy of the image, and scale it
  cvResize(initialImage, cr->scaledImage, CV_INTER_LINEAR);

  // convert the image to gray (maybe scaled?)
  //cvCvtColor(initialImage, gray, CV_BGR2GRAY);
  cvCvtColor(cr->scaledImage, cr->gray, CV_BGR2GRAY);

  // get the circles (note that this modifies the input image with Canny)
  return myHoughCircles(cr, cr->gray, clist);
}


GList *circlesFromImage2(circles_state_t *cr, IplImage *initialImage) {
   int w = initialImage->width;
   int h = initialImage->height;
   GList *circList;

   // possibly scale down
   if (w > 1024 || h > 768) {
     if (w > h) {
       cr->scale = 1024.0 / w;
     } else {
       cr->scale = 768.0 / h;
     }
     w *= cr->scale;
     h *= cr->scale;
   }

   printf("w: %d, h: %d, scale factor: %g\n",
	  initialImage->width,
	  initialImage->height,
	  cr->scale);

   // create where the scaled image will go
   cr->scaledImage = cvCreateImage(cvSize(w, h), 8, 3);

   // create the storage for the gray image used by the circle finder
   //gray = cvCreateImage(cvGetSize(initialImage), 8, 1);
   cr->gray = cvCreateImage(cvGetSize(cr->scaledImage), 8, 1);

   // do initial computation
   circList = computeCircles(cr, -1, initialImage, NULL);

   // free
   // XXX
   return circList;
}


// called from GUI
void circlesFromImage(IplImage *initialImage) {
  circles = circlesFromImage2(&staticState, initialImage);
}



// 3 functions for diamond filter interface

int f_init_circles (int num_arg, char **args,
		    int bloblen, void *blob_data,
		    const char *filter_name,
		    void **filter_args) {
  // TODO

  *filter_args = NULL;
  return 0;
}



int f_eval_circles (lf_obj_handle_t ohandle, void *filter_args) {
  // TODO

  return 100;
}



int f_fini_circles (void *filter_args) {
  // TODO

  return 0;
}
