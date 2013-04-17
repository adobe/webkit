This is a prototype of CSS Custom Filters integrated with the hardware accelerated pipeline in Chromium.

**Important**: The architecture used in this prototype does **not** reflect our current thinking of the best approach to implement hardware accelerated CSS Custom Filters.

This prototype renders CSS Custom Filters directly in the Chromium compositor. At the time of this writing, our thinking is that the Chromium compositor should delegate CSS Custom Filters rendering to Skia. This will make it easier to integrate CSS Custom Filters in Skia filter graphs.
