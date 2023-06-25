# DropletAnalysisCpp

Running:

from command prompt, run:
DropletAnalysis.exe "filename.csv" [-s, --frameskip INT] [-t, --threshold 0-255] [-o, --olympus]

Optional parameters:
-s or --frameskip [integer]: skip [integer] frames every time a frame is processed to speed up processing. If not supplied every frame of the video is processed.
-t or --threshold [0-255]: force pixels with brightness below the threshold to be black and pixels above threshold to be white. If not specified Otsu's method is used to determine threshold.
-o or --olympus: For videos from the olympus microscope without a scale line. Videos are assumed to be 1920x1200 pixels with 216 pixels being equal to 50 microns.

example:
DropletAnalysis.exe "C:\Test Videos\WP 30C 4POPC 1CHOL 187mosm sqe016.mp4" -s 10
DropletAnalysis.exe "C:\Test Videos\olympusvideo.avi" -s 10 -o

OpenCV Setup:
https://www.geeksforgeeks.org/opencv-c-windows-setup-using-visual-studio-2019/


OpenCV Hough Transform:
https://docs.opencv.org/3.4.15/d3/de5/tutorial_js_houghcircles.html

