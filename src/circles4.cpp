/*
 *  FatFind
 *  A Diamond application for adipocyte image exploration
 *  Version 1
 *
 *  Copyright (c) 2006 Carnegie Mellon University
 *  All Rights Reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#include <cstring>

#include <stdint.h>


#include "ltiRGBPixel.h"
#include "ltiScaling.h"
#include "ltiDraw.h"                  // drawing tool
#include "ltiALLFunctor.h"            // image loader
#include "ltiCannyEdges.h"            // edge detector
#include "ltiFastEllipseExtraction.h" // ellipse detector
#include "ltiGaussianPyramid.h"
#include "ltiSplitImageToHSV.h"

#include "circles4.h"
#include "util.h"

#include <sys/time.h>
#include <time.h>


// helper functions for glist
static gint circle_radius_compare(gconstpointer a,
				  gconstpointer b) {
  const circle_type *c1 = (circle_type *) a;
  const circle_type *c2 = (circle_type *) b;

  // assume a and b are positive
  double c1_radius = quadratic_mean_radius(c1->a, c1->b);
  double c2_radius = quadratic_mean_radius(c2->a, c2->b);
  if (c1_radius < c2_radius) {
    return 1;
  } else if (c2_radius < c1_radius) {
    return -1;
  } else {
    return 0;
  }
}


static void do_canny(lti::gaussianPyramid<lti::image> &imgPyramid,
		     std::vector<lti::channel8*> &edges,
		     double minSharpness) {
  uint64_t start_time_in_ms;
  uint64_t end_time_in_ms;

  struct timeval tv;

  gettimeofday(&tv, NULL);
  start_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  // params
  lti::cannyEdges::parameters cannyParam;
  if (minSharpness == 0) {
    cannyParam.thresholdMax = 0.04;
  } else {
    cannyParam.thresholdMax = 0.10 + 0.90 * ((minSharpness - 1) / 4.0);
  }
  cannyParam.thresholdMin = 0.04;
  cannyParam.kernelSize = 7;

  // run
  lti::cannyEdges canny(cannyParam);
  g_assert(edges.size() == 0);

  for (int i = 0; i < imgPyramid.size(); i++) {
    printf("canny[%d]: %dx%d\n", i, imgPyramid[i].columns(),
	   imgPyramid[i].rows());
    lti::channel8 *c8 = new lti::channel8;
    canny.apply(imgPyramid[i], *c8);
    edges.push_back(c8);
  }

  gettimeofday(&tv, NULL);
  end_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  printf("canny done in %lld ms\n", end_time_in_ms - start_time_in_ms);
}

static GList *do_fee(std::vector<lti::channel8*> &edges,
		     circles_state_t *ct) {
  GList *result = NULL;

  uint64_t start_time_in_ms;
  uint64_t end_time_in_ms;

  struct timeval tv;

  gettimeofday(&tv, NULL);
  start_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  // create FEE functor
  lti::fastEllipseExtraction::parameters feeParam;
  feeParam.maxArcGap = 120;
  feeParam.minLineLen = 3;

  for (unsigned int i = 0; i < edges.size(); i++) {
    double pyrScale = pow(2, i);

    lti::fastEllipseExtraction fee(feeParam);

    // extract some ellipses
    printf("extracting from pyramid %d\n", i);
    fee.apply(*edges[i]);

    // build list
    std::vector<lti::fastEllipseExtraction::ellipseEntry>
      &ellipses = fee.getEllipseList();

    for(unsigned int j=0; j<ellipses.size(); j++) {
      float a = ellipses[j].a * pyrScale;
      float b = ellipses[j].b * pyrScale;
      float r = quadratic_mean_radius(a, b);
      gboolean in_result = TRUE;

      // determine if it should go in by radius
      if (!(ct->minRadius < 0 || r >= ct->minRadius)) {
	in_result = FALSE;
      } else if (!(ct->maxRadius < 0 || r <= ct->maxRadius)) {
	in_result = FALSE;
      }

      // compute eccentricity
      float e = compute_eccentricity(a, b);

      // if over 0.9, drop completely
      if (e > 0.9) {
	continue;
      }

      // otherwise, maybe mark
      if (e > ct->maxEccentricity) {
	in_result = FALSE;
      }

      // all set
      circle_type *c = (circle_type *)g_malloc(sizeof(circle_type));

      c->x = ellipses[j].x * pyrScale;
      c->y = ellipses[j].y * pyrScale;
      c->a = ellipses[j].a * pyrScale;
      c->b = ellipses[j].b * pyrScale;
      c->t = ellipses[j].t;
      c->in_result = in_result;

      result = g_list_prepend(result, c);

      // print ellipse data
      printf(" ellipse[%i]: center=(%.0f,%.0f) semiaxis=(%.0f,%.0f) "
	     "angle=%.1f coverage=%.1f%% \n",
	     j, c->x, c->y, c->a, c->b, c->t*180/M_PI,
	     ellipses[j].coverage*100);
    }
  }

  gettimeofday(&tv, NULL);
  end_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  printf("fee done in %lld ms\n", end_time_in_ms - start_time_in_ms);

  return result;
}

static circles_state_t staticState = {-1, -1, 0.4, 1};
GList *circlesFromImage2(circles_state_t *ct,
			 const int width, const int height,
			 const int stride, const int bytesPerPixel,
			 void *data) {
  uint64_t start_time_in_ms;
  uint64_t end_time_in_ms;

  struct timeval tv;

  gettimeofday(&tv, NULL);
  start_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  // load image
  g_assert(bytesPerPixel >= 3);
  lti::image img(false, height, width);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      guchar *d = ((guchar *) data) + (x * bytesPerPixel + y * stride);
      lti::rgbPixel p(d[0], d[1], d[2]);
      img[y][x] = p;
    }
  }

  // make pyramid
  int levels = (int)log2(MIN(img.rows(), img.columns()));
  lti::gaussianPyramid<lti::image> imgPyramid(levels);
  imgPyramid.generate(img);

  gettimeofday(&tv, NULL);
  end_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  printf("load/pyramid done in %lld ms\n", end_time_in_ms - start_time_in_ms);

  // make vector
  std::vector<lti::channel8*> edges;

  // run
  do_canny(imgPyramid, edges, ct->minSharpness);
  GList *result = do_fee(edges, ct);

  // clear vector
  for (unsigned int i = 0; i < edges.size(); i++) {
    delete edges[i];
    edges[i] = NULL;
  }

  // overlap suppression
  printf("overlap suppression ");
  fflush(stdout);

  gettimeofday(&tv, NULL);
  start_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  result = g_list_sort(result, circle_radius_compare);  // sort

  GList *iter;
  for (iter = result; iter != NULL; iter = g_list_next(iter)) {
    // find other centers within this circle (assume no eccentricity)
    circle_type *c1 = (circle_type *) iter->data;
    double c1_radius = quadratic_mean_radius(c1->a, c1->b);

    GList *iter2;

    // don't bother if large eccentricity
    double c1_e = compute_eccentricity(c1->a, c1->b);
    if (c1_e > 0.4) {
      continue;
    }

    iter2 = g_list_next(iter);
    while (iter2 != NULL) {
      circle_type *c2 = (circle_type *) iter2->data;
      double c2_radius = quadratic_mean_radius(c2->a, c2->b);

      // is c2 within c1?
      double xd = c2->x - c1->x;
      double yd = c2->y - c1->y;
      double dist = sqrt((xd * xd) + (yd * yd));

      // maybe delete?
      GList *next = g_list_next(iter2);
      //      printf(" dist: %g, c1_radius: %g, c2_radius: %g\n",
      //	     dist, c1_radius, c2_radius);

      // XXX fudge
      if ((dist + c2_radius) < (c1_radius + 100)) {
	printf("x");
	fflush(stdout);
	result = g_list_delete_link(result, iter2);
      }

      iter2 = next;
    }
    printf(".");
    fflush(stdout);
  }
  printf("\n");

  gettimeofday(&tv, NULL);
  end_time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  printf("overlap suppression done in %lld ms\n", end_time_in_ms - start_time_in_ms);

  return result;
}


// called from GUI
GList *circlesFromImage(const int width, const int height,
			const int stride, const int bytesPerPixel,
			void *data, double minSharpness) {
  circles_state_t cs = staticState;
  cs.minSharpness = minSharpness;
  return circlesFromImage2(&cs, width,
			   height, stride, bytesPerPixel, data);
}
