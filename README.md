# DropletAnalysisCpp

Running:

from command prompt, run:
DropletAnalysis.exe "filename.csv" [frameskip] [threshold 0-255]

For videos from the olympus microscope without a scale line, add "-o" or "--olympus" to the command line:
DropletAnalysis.exe "filename.csv" [frameskip] [threshold 0-255] -o

example:
DropletAnalysis.exe "C:\Test Videos\WP 30C 4POPC 1CHOL 187mosm sqe016.mp4" 10 25

OpenCV Setup:
https://www.geeksforgeeks.org/opencv-c-windows-setup-using-visual-studio-2019/


OpenCV Hough Transform:
https://docs.opencv.org/3.4.15/d3/de5/tutorial_js_houghcircles.html

