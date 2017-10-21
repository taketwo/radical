# `calibrate_vignetting_response`

This app allows you to calibrate the vignetting response of the camera.

Get a sheet of white paper and fix it on a table so it is uniformly lit by the
light sources in your office. If your table has a bright color, it is better to
make a contour around the paper with black tape to make sure that the blob
detector is able to distinguish between the white paper and the table.

Point the camera at the paper and start the app. Adjust the exposure time with
up and down arrows. We want the exposure time to be as large as possible,
however there should be no saturated pixels. For your convenience, overexposed
pixels will be highlighted with red color. After adjusting the exposure time so
that there are no red pixels on the calibration target, press Enter to start
data collection. Move the camera around so that the paper is observed through
every camera pixel. Take care to move the camera so that it does not cast
shadows onto the paper. In the bottom-left corner the amount of collected
samples per pixels is visualized. By default, we aim to collect 100 samples per
pixel. Once this amount is collected, the corresponding pixel will turn green.
After the required number of samples is collected for every pixel, the
vignetting response will be calibrated and stored in a file named after the
camera (model type + serial number). Run with `--help` to see different options.
