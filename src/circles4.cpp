#include "ltiRGBPixel.h"
#include "ltiScaling.h"
#include "ltiDraw.h"                  // drawing tool
#include "ltiALLFunctor.h"            // image loader
#include "ltiCannyEdges.h"            // edge detector
#include "ltiFastEllipseExtraction.h" // ellipse detector
#include "ltiGaussianPyramid.h"
#include "ltiSplitImageToHSV.h"

#include "circles4.h"
#include "lib_filter.h"

GList *circles;   // for gui only, not filter


typedef struct {
  int minRadius;
  int maxRadius;
} circles_state_t;


// helper functions for glist
static void free_1_circle(gpointer data, gpointer user_data) {
  g_free((circle_type *) data);
}

static gint circle_radius_compare(gconstpointer a,
				  gconstpointer b) {
  const circle_type *c1 = (circle_type *) a;
  const circle_type *c2 = (circle_type *) b;

  // assume a and b are positive
  double c1_radius = c1->a + c1->b;
  double c2_radius = c2->a + c2->b;
  if (c1_radius < c2_radius) {
    return 1;
  } else if (c2_radius < c1_radius) {
    return -1;
  } else {
    return 0;
  }
}


static void do_canny(lti::gaussianPyramid<lti::image> &imgPyramid,
		     std::vector<lti::channel8*> &edges) {
  // params
  lti::cannyEdges::parameters cannyParam;
  cannyParam.thresholdMax = 0.10;
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
}

static GList *do_fee(std::vector<lti::channel8*> &edges,
		     circles_state_t *ct) {
  GList *result = NULL;

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

      // determine if it should go in by radius
      if (!(ct->minRadius < 0 || a >= ct->minRadius || b >= ct->minRadius)) {
	continue;
      }
      if (!(ct->maxRadius < 0 || a <= ct->maxRadius || b <= ct->maxRadius)) {
	continue;
      }

      // compute eccentricity
      if (b > a) {
	float c = a;
	a = b;
	b = c;
      }
      float e = sqrt(1 - ((b*b) / (a*a)));

      // XXX magic number
      if (e > 0.4) {
	continue;
      }

      // all set
      circle_type *c = (circle_type *)g_malloc(sizeof(circle_type));

      c->x = ellipses[j].x * pyrScale;
      c->y = ellipses[j].y * pyrScale;
      c->a = ellipses[j].a * pyrScale;
      c->b = ellipses[j].b * pyrScale;
      c->t = ellipses[j].t;

      result = g_list_prepend(result, c);

      // print ellipse data
      printf(" ellipse[%i]: center=(%.0f,%.0f) semiaxis=(%.0f,%.0f) "
	     "angle=%.1f coverage=%.1f%% \n",
	     j, c->x, c->y, c->a, c->b, c->t*180/M_PI,
	     ellipses[j].coverage*100);
    }
  }

  return result;
}

static circles_state_t staticState = {-1, -1};
static GList *circlesFromImage2(circles_state_t *ct,
				const int width, const int height,
				const int stride, const int bytesPerPixel,
				void *data) {
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

  // make vector
  std::vector<lti::channel8*> edges;

  // run
  do_canny(imgPyramid, edges);
  GList *result = do_fee(edges, ct);

  // clear vector
  for (unsigned int i = 0; i < edges.size(); i++) {
    delete edges[i];
    edges[i] = NULL;
  }

  // overlap supression
  printf("overlap supression ");
  fflush(stdout);

  result = g_list_sort(result, circle_radius_compare);  // sort

  GList *iter = result;
  while (iter != NULL) {
    // find other centers within this circle (assume no eccentricity)
    circle_type *c1 = (circle_type *) iter->data;
    double c1_radius = (c1->a + c1->b) / 2.0;

    GList *iter2 = g_list_next(iter);
    while (iter2 != NULL) {
      circle_type *c2 = (circle_type *) iter2->data;
      double c2_radius = (c2->a + c2->b) / 2.0;

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
    iter = g_list_next(iter);
  }
  printf("\n");

  return result;
}


// called from GUI
void circlesFromImage(const int width, const int height,
		      const int stride, const int bytesPerPixel,
		      void *data) {
  circles = circlesFromImage2(&staticState, width,
			      height, stride, bytesPerPixel, data);
}



// 3 functions for diamond filter interface

extern "C" {
  int f_init_circles (int num_arg, char **args,
		      int bloblen, void *blob_data,
		      const char *filter_name,
		      void **filter_args) {
    circles_state_t *cr;

    // check parameters
    if (num_arg != 2) {
      return -1;
    }

    // init state
    cr = (circles_state_t *)g_malloc(sizeof(circles_state_t));

    cr->minRadius = g_ascii_strtoull(args[0], NULL, 10);
    cr->maxRadius = g_ascii_strtoull(args[1], NULL, 10);

    // we're good
    *filter_args = cr;
    return 0;
  }



  int f_eval_circles (lf_obj_handle_t ohandle, void *filter_args) {
    // circles
    GList *clist;
    circles_state_t *cr = (circles_state_t *) filter_args;
    int num_circles;

    // for attributes from diamond
    size_t len;
    unsigned char *data;



    // get the data from the ohandle, convert to IplImage
    int w;
    int h;

    // width
    lf_ref_attr(ohandle, "_rows.int", &len, &data);
    h = *((int *) data);

    // height
    lf_ref_attr(ohandle, "_cols.int", &len, &data);
    w = *((int *) data);

    // image data
    lf_ref_attr(ohandle, "_rgb_image.rgbimage", &len, &data);

    // feed it to our processor
    clist = circlesFromImage2(cr, w, h, w * 4, 4, data);

    data = NULL;

    // add the list of circles to the cache and the object
    // XXX
    num_circles = g_list_length(clist);
    if (num_circles > 0) {
      GList *l = clist;
      int i = 0;
      data = (unsigned char *) g_malloc(sizeof(float) * 5 * num_circles);

      // pack in
      while (l != NULL) {
	float *p = ((float *) data) + i;
	circle_type *c = (circle_type *) l->data;
	p[0] = c->x;
	p[1] = c->y;
	p[2] = c->a;
	p[3] = c->b;
	p[4] = c->t;

	i += 5;
	l = g_list_next(l);
      }
      lf_write_attr(ohandle, "circle-data", sizeof(float) * 5 * num_circles, data);
    }

    // free others
    g_list_foreach(clist, free_1_circle, NULL);
    g_list_free(clist);
    clist = NULL;
    g_free(data);
    data = NULL;

    // return number of circles
    return num_circles;
  }



  int f_fini_circles (void *filter_args) {
    g_free((circles_state_t *) filter_args);

    return 0;
  }
}
