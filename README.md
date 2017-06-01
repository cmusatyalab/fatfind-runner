# fatfind-runner
Adipocyte filter for FatFind

[FatFind](https://github.com/cmusatyalab/fatfind) is an application for
adipocyte investigation using
[Diamond: Interactive Search of Non-Indexed Data](http://diamond.cs.cmu.edu).

This repository contains the filter executable that runs on the Diamond servers.

## Building

This filter depends on
[LTI-Lib](http://ltilib.sourceforge.net/doc/homepage/index.shtml). If you get
compile errors about `undefined resize()` functions revision 3360 in SVN seems
to have the proper fixes.

If compilation still fails because of undefined libpng symbol errors, apply the
following patch to ltilib.

```diff
--- ltilib/src/io/ltiPNGLibFunctor.cpp.orig     2017-05-30 19:32:21.600730711 +0000
+++ ltilib/src/io/ltiPNGLibFunctor.cpp  2017-05-30 19:36:30.351134377 +0000
@@ -350,7 +350,7 @@
              colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
       // convert 1, 2, 4 bit to 8 bit
       if (bitDepth < 8) {
-        png_set_gray_1_2_4_to_8(pngPtr);
+        png_set_expand_gray_1_2_4_to_8(pngPtr);
       }
       else if (!(bitDepth == 8 || bitDepth == 16)) {
         setStatusString("Invalid bit depth in grayscale image");
@@ -513,10 +513,7 @@
     png_read_info(pngPtr, infoPtr);
     png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType,
                  NULL, NULL, NULL);
-
-    // get the palette of the image from the infoPtr directly
-    numPalette = infoPtr->num_palette;
-    pngPalette = infoPtr->palette;
+    png_get_PLTE(pngPtr, infoPtr, &pngPalette, &numPalette);
 
     // convert images and handle palette in 2 different cases below,
     // grayscale or palette data grayscale image convertion
@@ -526,7 +523,7 @@
 
         if (bitDepth < 8) {
           // convert 1, 2, 4 bit grayscale to 8 bit
-          png_set_gray_1_2_4_to_8(pngPtr);
+          png_set_expand_gray_1_2_4_to_8(pngPtr);
         }
         else if (bitDepth == 16) {
           // strip 16 bit grayscale to 8 bit
```

If binaries fail to link with the resulting library, with an `undefined MAIN__`
error. Use the following patch as a workaround. Apply patch, rerun configure
and rebuild/reinstall lti-lib.

```diff
--- ltilib/linux/lti-config.in.orig  2017-06-01 17:40:07.202911793 +0000
+++ ltilib/linux/lti-config.in   2017-06-01 17:40:22.247308190 +0000
@@ -10,7 +10,7 @@
 # Check for LAPACK
 lapack_libs=@LAPACK_LIBS@
 if test -n "$lapack_libs" ; then
-LIBS="@LAPACK_LIBS@ @BLAS_LIBS@ @F2C_LIBS@ @LIBS@ @FLIBS@"
+LIBS="@LAPACK_LIBS@ @BLAS_LIBS@ -Wl,--undefined=MAIN__ @F2C_LIBS@ @LIBS@ @FLIBS@"
 else
 LIBS="@LIBS@" 
 fi
--- ltilib/linux/lti-local-config.in.orig    2017-06-01 18:08:51.785301013 +0000
+++ ltilib/linux/lti-local-config.in 2017-06-01 18:09:08.577750454 +0000
@@ -9,7 +9,7 @@
 # Check for LAPACK
 lapack_libs=@LAPACK_LIBS@
 if test -n "$lapack_libs" ; then
-LIBS="@LAPACK_LIBS@ @BLAS_LIBS@ @F2C_LIBS@ @LIBS@ @FLIBS@"
+LIBS="@LAPACK_LIBS@ @BLAS_LIBS@ -Wl,--undefined=MAIN__ @F2C_LIBS@ @LIBS@ @FLIBS@"
 lapack_include="-I${prefix}/misc/lamath"
 else
 LIBS="@LIBS@"
```
