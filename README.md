may2012-f2f-prototype
=====================

`may2012-f2f-prototype` branch contains various changes intended for presentation at the May 2012 CSS WG F2F meeting in Hamburg. This branch includes prototype and experimental work. It should not be considered final or ready for submission into the WebKit trunk.

General Changes
---------------


CSS Regions
-----------

CSS Exclusions
-----------

CSS Shaders
-----------
Most of the CSS Shaders code is already upstreamed to WebKit. We are looking forward to upstreaming all the CSS Shaders changes in the following weeks.

List of CSS Shaders changes in this branch:

+ [71443](https://bugs.webkit.org/show_bug.cgi?id=71443) - Added support for the "transform" parameters

+ [85113](https://bugs.webkit.org/show_bug.cgi?id=85113) - Make CSS Shaders render to texture framebuffers

+ [85086](https://bugs.webkit.org/show_bug.cgi?id=85086) - [CSS Shaders] Improve performance of CSS Shaders rendering - Partial implementation, sharing the shaders and the GraphicsContext3D between filtered elements.

+ Prototyped a way to rewrite the shaders, so that they don't get access to the element's texture. This is a recommended security measure to avoid timming attacks. For that we used the ANGLE project and all the changes are available in our [ANGLE branch](https://github.com/adobe/angle/tree/shader-rewriter).

CSS Compositing
-----------


