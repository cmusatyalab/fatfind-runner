#include <cv.h>
#include <highgui.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

// parameters
static int dp = 2;
static int minDist = 0;
static int blur = 2;

static int accumulatorThresh = 600;
static const int accumulatorMax = 1000;

static int minRadius = 5;
static int maxRadius = 100;
static int radiusStep = 4;

static int cannyThreshold = 100;

// scale factor for image
static double scale = 1.0;


// image storage
static IplImage* initialImage;
static IplImage* scaledImage;
static IplImage* gray;

static CvMemStorage* storage;


static void addToMatrix (CvMat *m, int x, int y, double amount) {
  CvScalar s;

  if (x < 0 || y < 0 || x >= m->rows || y >= m->cols) {
    return;
  }
  // amount = 1;      // for debugging only
  m->data.fl[x * m->cols + y] += amount;
  /*
  s = cvGet2D(m, x, y);
  s.val[0] += amount;
  cvSet2D(m, x, y, s);
  */
}

static void circlePoints(CvMat *img, int cx, int cy, int x, int y, double addAmount)
{
  if (x == 0) {
    addToMatrix(img, cx, cy + y, addAmount);
    addToMatrix(img, cx, cy - y, addAmount);
    addToMatrix(img, cx + y, cy, addAmount);
    addToMatrix(img, cx - y, cy, addAmount);
  } else if (x == y) {
    addToMatrix(img, cx + x, cy + y, addAmount);
    addToMatrix(img, cx - x, cy + y, addAmount);
    addToMatrix(img, cx + x, cy - y, addAmount);
    addToMatrix(img, cx - x, cy - y, addAmount);
  } else if (x < y) {
    addToMatrix(img, cx + x, cy + y, addAmount);
    addToMatrix(img, cx - x, cy + y, addAmount);
    addToMatrix(img, cx + x, cy - y, addAmount);
    addToMatrix(img, cx - x, cy - y, addAmount);
    addToMatrix(img, cx + y, cy + x, addAmount);
    addToMatrix(img, cx - y, cy + x, addAmount);
    addToMatrix(img, cx + y, cy - x, addAmount);
    addToMatrix(img, cx - y, cy - x, addAmount);
  }
}

static void accumulateCircle(CvMat *img, int xCenter, int yCenter, int radius)
{
  int x = 0;
  int y = radius;
  int p = (5 - radius*4)/4;


  double addAmount = 1;

  //printf("radius: %d, addAmount: %g\n", radius, addAmount);

  circlePoints(img, xCenter, yCenter, x, y, addAmount);
  while (x < y) {
    x++;
    if (p < 0) {
      p += 2*x+1;
    } else {
      y--;
      p += 2*(x-y)+1;
    }
    circlePoints(img, xCenter, yCenter, x, y, addAmount);
  }
}

static CvSeq* myHoughCircles (IplImage *image, void* circle_storage) {
  // storage for the circles
  CvSeq *circles = cvCreateSeq(CV_32FC3, sizeof(CvSeq),
			       sizeof(float)*3, (CvMemStorage*)circle_storage);

  const int numSlices = (maxRadius - minRadius) / (radiusStep + 1);

  CvMat **acc;
  int i, j, x, y;

  const int w = image->width / dp;
  const int h = image->height / dp;

  IplImage *houghScaledImage = cvCreateImage(cvSize(w, h), 8, 1);
  cvResize(image, houghScaledImage, CV_INTER_NN);


  // create accumulator stack
  acc = (CvMat**) cvAlloc(sizeof(CvMat*) * numSlices);
  //  printf("allocating slices ");
  //fflush(stdout);
  for (i = 0; i < numSlices; i++) {
    acc[i] = cvCreateMat(w, h, CV_32FC1);
    //printf(" %4d", i);
    //fflush(stdout);

    cvZero(acc[i]);
    //printf("z");
    //fflush(stdout);
  }

  // do canny
  cvSmooth(image, image, CV_GAUSSIAN, blur*2+1, blur*2+1, 0);
  //  cvCanny(image, image, MAX(cannyThreshold/4,1), cannyThreshold, 3);
  cvCanny(image, image, 0, cannyThreshold, 3);
  //  cvResize(houghScaledImage, image, CV_INTER_NN);

  // for each pixel in the canny, draw into the accumulator for each radius
  for (y = h - 1; y >= 0; y--) {
    for (x = w - 1; x >= 0; x--) {
      // is there an edge at this point?
      if (image->imageData[y * dp * image->widthStep + x * dp]) {
      //      if (cvGet2D(image, y * dp, x * dp).val[0]) {
	for (i = 0; i < numSlices; i++) {
	  CvMat *slice = acc[i];
	  int radius = minRadius + ((radiusStep + 1) * i);

	  //printf("drawing circle: (%d,%d,%d)\n", x, y, radius);
	  accumulateCircle(slice, x, y, radius);
	}
      }
    }
  }


  // walk over accumulator, eliminate anything < threshold
  for (i = 0; i < numSlices; i++) {
    CvMat *slice = acc[i];
    int radius = minRadius + ((radiusStep + 1) * i);
    double circ = 2.0 * M_PI * radius;
    double thresh = circ * ((double) accumulatorThresh) / ((double) accumulatorMax);
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
	if (slice->data.fl[y * slice->rows + x] < thresh) {
	  slice->data.fl[y * slice->rows + x] = 0.0;
	}
      }
    }
    //cvThreshold(slice, slice, thresh, 0, CV_THRESH_TOZERO);
    nonZero = cvCountNonZero(slice);
    printf("slice: %d, radius: %d, thresh: %g, nonZero: %d\n", i, radius, thresh, nonZero);
  }


#define MAXIMA_BOTTOM_UP 1

  // walk over accumulator again, searching for maxima and wiping out regions around them
#if MAXIMA_BOTTOM_UP
  for (i = 0; i < numSlices; i++) {
#else
  for (i = numSlices - 1; i >= 0; i--) {
#endif
    CvMat *slice = acc[i];
    double min_val, max_val;
    CvPoint max_loc;
    int radius = minRadius + ((radiusStep + 1) * i);

    cvMinMaxLoc(slice, &min_val, &max_val, NULL, &max_loc, NULL);
    while(max_val > 0) {
      // found a point
      float *p = cvAlloc(sizeof(float) * 3);
      int x = max_loc.y;
      int y = max_loc.x;

      //      printf("circle found; r: %d\n", radius);

      p[0] = x * dp;
      p[1] = y * dp;
      p[2] = radius * dp;
      cvSeqPush(circles, p);

      // wipe out region in all slices
#if MAXIMA_BOTTOM_UP
      for (j = i; j < numSlices; j++) {
#else
      for (j = 0; j <= i; j++) {
#endif
	int newRadius = minRadius + ((radiusStep + 1) * j);
	cvCircle(acc[j], max_loc, minDist + newRadius, cvScalarAll(0), -1, 8, 0);
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

  return circles;
}

static void computeCircles(int pos) {
  CvSeq* circles;
  IplImage* tmpGrayImage = cvCreateImage(cvGetSize(scaledImage), 8, 3);
  IplImage* scaledGray = cvCreateImage(cvGetSize(scaledImage), 8, 1);

  // crappy bounds of highgui
  if (minRadius <= 0) {
    minRadius = 1;
  }

  // get a fresh copy of the image, and scale it
  cvResize(initialImage, scaledImage, CV_INTER_LINEAR);

  // convert the image to gray (maybe scaled?)
  //cvCvtColor(initialImage, gray, CV_BGR2GRAY);
  cvCvtColor(scaledImage, gray, CV_BGR2GRAY);

  // get the circles (note that this modifies the input image with Canny)
  circles = myHoughCircles(gray, storage);
  int i;

  // resize the now-Cannified input image
  cvResize(gray, scaledGray, CV_INTER_AREA);

  // convert the resized image to color so we can overlay the edges
  cvCvtColor(scaledGray, tmpGrayImage, CV_GRAY2BGR);

  // overlay the found edges onto the image for the screen
  cvAdd(tmpGrayImage, scaledImage, scaledImage, NULL);
  cvReleaseImage(&tmpGrayImage);

  // draw the circles!
  for(i = 0; i < circles->total; i++) {
    float* p = (float*)cvGetSeqElem(circles, i);
    //printf("%.10d %g %g %g\n",i, p[0], p[1], p[2]);
    /*
    cvCircle(scaledImage, cvPoint(cvRound(p[0]*scale),cvRound(p[1]*scale)), 3, CV_RGB(0,255,0), -1, 8, 0);
    cvCircle(scaledImage, cvPoint(cvRound(p[0]*scale),cvRound(p[1]*scale)), cvRound(p[2]*scale), CV_RGB(255,0,0), 1, 8, 0);
    */
    cvCircle(scaledImage, cvPoint(cvRound(p[0]),cvRound(p[1])), 3, CV_RGB(0,255,0), -1, 8, 0);
    cvCircle(scaledImage, cvPoint(cvRound(p[0]),cvRound(p[1])), cvRound(p[2]), CV_RGB(255,0,0), 1, 8, 0);
    //cvFree(&p);
  }

  cvClearSeq(circles);

  // finally, show the image
  cvShowImage("circles", scaledImage);
}


int main(int argc, char** argv) {
  if(argc == 2 && (initialImage=cvLoadImage(argv[1], 1))!= 0) {
    int w = initialImage->width;
    int h = initialImage->height;

    // possibly scale down
    if (w > 1024 || h > 768) {
      if (w > h) {
	scale = 1024.0 / w;
      } else {
	scale = 768.0 / h;
      }
      w *= scale;
      h *= scale;
    }

    printf("w: %d, h: %d, scale factor: %g\n", initialImage->width, initialImage->height, scale);

    // create where the scaled image will go
    scaledImage = cvCreateImage(cvSize(w, h), 8, 3);

    // create the storage for the gray image used by the circle finder
    //gray = cvCreateImage(cvGetSize(initialImage), 8, 1);
    gray = cvCreateImage(cvGetSize(scaledImage), 8, 1);

    storage = cvCreateMemStorage(0);

    // setup the "gui"
    cvNamedWindow("circles", 1);
    cvCreateTrackbar("dp", "circles",
		      &dp, 8, computeCircles);
    cvCreateTrackbar("min dist", "circles",
		      &minDist, 1000, computeCircles);
    cvCreateTrackbar("canny threshold", "circles",
		      &cannyThreshold, 1000, computeCircles);
    cvCreateTrackbar("accumulator", "circles",
		      &accumulatorThresh, accumulatorMax, computeCircles);
    cvCreateTrackbar("min radius", "circles",
		      &minRadius, 1000, computeCircles);
    cvCreateTrackbar("max radius", "circles",
		      &maxRadius, 1000, computeCircles);
    cvCreateTrackbar("radius step", "circles",
		      &radiusStep, 20, computeCircles);
    cvCreateTrackbar("blur", "circles",
		      &blur, 24, computeCircles);

    // do initial computation
    computeCircles(-1);

    // wait for input from user
    while (1) {
      cvWaitKey(0);
    }
  } else {
    printf("no image?\n");
  }

  return 0;
}
