#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "ltiRGBPixel.h"
#include "ltiScaling.h"
#include "ltiDraw.h"                  // drawing tool
#include "ltiALLFunctor.h"            // image loader
#include "ltiCannyEdges.h"            // edge detector
#include "ltiFastEllipseExtraction.h" // ellipse detector
#include "ltiScaleSpacePyramid.h"
#include "ltiSplitImageToHSV.h"

#include "circles4.h"
#include "lib_filter.h"

GList *circles;   // for gui only, not filter


typedef struct {
  lti::image inputImg;
  lti::scaleSpacePyramid<lti::image> imgPyramid;
  lti::image scaledInputImg;
  std::vector<lti::channel8> edges;
  lti::channel8 scaledEdges;
  lti::image result;

  int minRadius;
  int maxRadius;

  std::vector<lti::fastEllipseExtraction> fee;
} circles_state_t;


// helper function for glist
static void free_1_circle(gpointer data, gpointer user_data) {
  g_free((circle_type *) data);
}


static void do_canny(circles_state_t *ct) {
  // params
  lti::cannyEdges::parameters cannyParam;
  cannyParam.thresholdMax = 0.10;
  cannyParam.thresholdMin = 0.04;
  cannyParam.kernelSize = 7;

  // run
  lti::cannyEdges canny(cannyParam);
  ct->edges.clear();
  for (int i = 0; i < ct->imgPyramid.size(); i++) {
    printf("canny[%d]: %dx%d\n", i, ct->imgPyramid[i].columns(),
	   ct->imgPyramid[i].rows());
    ct->edges.push_back(lti::channel8());
    canny.apply(ct->imgPyramid[i], ct->edges[i]);
  }
}

static GList *do_fee(circles_state_t *ct) {
  GList *result = NULL;

  // create FEE functor
  lti::fastEllipseExtraction::parameters feeParam;
  feeParam.maxArcGap = 120;
  feeParam.minLineLen = 3;

  ct->fee.clear();
  for (unsigned int i = 0; i < ct->edges.size(); i++) {
    lti::fastEllipseExtraction fee2(feeParam);
    ct->fee.push_back(fee2);

    // extract some ellipses
    ct->fee[i].apply(ct->edges[i]);

    // build list
    std::vector<lti::fastEllipseExtraction::ellipseEntry>
      &ellipses = ct->fee[i].getEllipseList();

    for(unsigned int j=0; i<ellipses.size(); i++) {
      circle_type *c = (circle_type *)g_malloc(sizeof(circle_type));

      c->x = ellipses[j].x;
      c->y = ellipses[j].y;
      c->a = ellipses[j].a;
      c->b = ellipses[j].b;
      c->t = ellipses[j].t;

      result = g_list_prepend(result, c);

      // print ellipse data
      printf(" ellipse[%i]: center=(%.0f,%.0f) semiaxis=(%.0f,%.0f) "
	     "angle=%.1f coverage=%.1f%% \n",
	     j, ellipses[j].x, ellipses[j].y,
	     ellipses[j].a, ellipses[j].b,
	     ellipses[j].t*180/3.14, ellipses[j].coverage*100);
    }
  }

  return result;
}

static circles_state_t staticState;
static GList *circlesFromImage2(circles_state_t *ct,
				const int width, const int height,
				const int stride, void *data) {
  // load image
  lti::image img(false, height, width);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      guchar *d = ((guchar *) data) + (x * 3 + y * stride);
      lti::rgbPixel p(d[0], d[1], d[2]);
      img[y][x] = p;
    }
  }
  ct->inputImg = img;

  // make pyramid
  lti::scaleSpacePyramid<lti::image>::parameters pyramidParams;
  pyramidParams.factor = 0.5;
  int levels = (int)log2(MIN(ct->inputImg.rows(), ct->inputImg.columns()));
  ct->imgPyramid = lti::scaleSpacePyramid<lti::image>(levels, pyramidParams);
  ct->imgPyramid.generate(ct->inputImg);

  // initial run
  do_canny(ct);
  return do_fee(ct);
}


// called from GUI
void circlesFromImage(const int width, const int height,
		      const int stride, void *data) {
  circles = circlesFromImage2(&staticState, width,
			      height, stride, data);
}



// 3 functions for diamond filter interface

int f_init_circles (int num_arg, char **args,
		    int bloblen, void *blob_data,
		    const char *filter_name,
		    void **filter_args) {
  circles_state_t *cr;

  // check parameters
  if (num_arg != 8) {
    return -1;
  }

  // init state
  cr = new circles_state_t;

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
  w = *((int *) data);

  // height
  lf_ref_attr(ohandle, "_cols.int", &len, &data);
  h = *((int *) data);

  // image data
  lf_ref_attr(ohandle, "_rgb_image.rgbimage", &len, &data);

  // feed it to our processor
  clist = circlesFromImage2(cr, w, h, w * 3, data);

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
  delete((circles_state_t *) filter_args);

  return 0;
}
