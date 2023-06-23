
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>

#define MATH_PI 3.14159265


using namespace std;
using namespace cv;

bool g_scaleSet = false;
bool g_firstframe = true;
float g_pixelsPerUnit = 0.0f;
int g_scalePixels = 0;
int g_scaleUnits = 50;
int g_framesPerSecond = 30;
int g_currentFrame = 0;
string g_unitType = "microns";
ofstream g_outfile;
int g_threshold = -1;
int g_frameskip = 1;


int g_scaleFrameStartX, g_scaleFrameStartY, g_scaleFrameEndX, g_scaleFrameEndY;

int g_scaleLineLength = 0;
int g_scaleLineStartR, g_scaleLineStartC, g_scaleLineEndC;

float sqDistance(Point p1, Point p2);
Vec4i getSeparatorLine(Point p1, Point p2);
float getSlope(Vec4i line);
bool circlesInsideEachOther(Vec3i c1, Vec3i c2);


//Time Stamp	 Adjusted Time	 Droplet 1 Radius	 Droplet 1 Volume	 Droplet 2 Radius	 Droplet 2 Volume	 Total Volume	 DIB Radius	 Contact Angle	 Radial Distance
//Takes circles in pixels, conversion to units done within this method
bool process(Vec3i c1, Vec3i c2) {
    float gv, sv, tv;
    float gr, sr;
    float dibRadius, rdib, radialDistance;
    float contactAngle;
    float c1h, c2h, i1x, i1y, i2x, i2y;
    float iDistance;

    cout << endl;

    float timeStamp = (float)g_currentFrame / (float)g_framesPerSecond;
    /*DEBUG*/ cout << "T=" << timeStamp << "\t";

    bool twoDroplets = true;							//NOT redundant, default, see below
    float rDistance = (float)sqrt((c2[0] - c1[0]) * (c2[0] - c1[0]) + (c2[1] - c1[1]) * (c2[1] - c1[1]));
    /*DEBUG*/ cout << rDistance << "jp = " << (rDistance / g_pixelsPerUnit) << " " << g_unitType << "\t";
    if ((rDistance * rDistance) >= ((c1[2] + c2[2]) * (c1[2] + c2[2]))) {
        cout << "Two unconnected droplets\t";
        gv = (4.0f / 3.0f) * MATH_PI * (float)((long)c1[2] * (long)c1[2] * (long)c1[2]);
        sv = (4.0f / 3.0f) * MATH_PI * (float)((long)c2[2] * (long)c2[2] * (long)c2[2]);
        //InformationPanel.volumeLabel.setText(to_string(gv + sv));
        tv = gv + sv;
        return true;
    }
    if ((rDistance * rDistance) <= ((c1[2] - c2[2]) * (c1[2] - c2[2]))) twoDroplets = false;

    /*DEBUG*/	if (twoDroplets) cout << "Two connected droplets\t";
    /*DEBUG*/	else cout << "One droplet with contact bilayer\t";

    float small = c1[2]; float big = c2[2];
    // Not safe at this point to have computer decide which droplet is growing and which is shrinking
    //if (c2[2] < c1[2]) {
    //    small = c2[2]; big = c1[2]; //}
    //    ///*DEBUG*/			cout << "Switched circles\n";
    //}
    /////*DEBUG*/			else cout << "No switch\n";
    big /= g_pixelsPerUnit;	small /= g_pixelsPerUnit;	//needed for volume calculations

    if (twoDroplets) {	//circle-circle intersection
        //first correct distance for droplet orientation
        float lf = rDistance / g_pixelsPerUnit;
        // lf is apparent distance, lr is distance after compensation for 3d
        float lr = (float)sqrt((big - small) * (big - small) + lf * lf);
        radialDistance = lr;
        //InformationPanel.distanceLabel.setText(to_string(lr));
        //then do math
                            //the following is from that droplet shape paper
        float thetab = (float)acos(
            (lr * lr - (small * small + big * big))
            / (2.0f * small * big)
        );
        float rdib = (small * big * (float)sin(thetab)) / lr;
        //InformationPanel.dibRadius.setText(to_string(rdib));// / pixelsPerUnit));
        dibRadius = rdib;
        float theta_degrees = 180.0f * thetab / (float)MATH_PI; theta_degrees /= 2.0f;
        //InformationPanel.contactAngle.setText(to_string(theta_degrees));
        contactAngle = theta_degrees;
        //Dr. Lee wants half of the contact angle
        ///*DEBUG*/	cout << "New Method: " << rdib << ", " << to_string(180.0f * thetab / (float)MATH_PI) << endl;
        //this is circle-circle intersection viewing from above
        float lr_pixels = lr * g_pixelsPerUnit;
        //this will cause the line denoting the circle-circle intersection to appear wrong
        float a = ((float)(c1[2] * c1[2]) - (float)(c2[2] * c2[2]) + (lr_pixels * lr_pixels)) / (2.0f * lr_pixels);
        float hi = (float)sqrt(c1[2] * c1[2] - a * a); float b = lr_pixels - a;
        c1h = ((float)c1[2] - a) / g_pixelsPerUnit;
        c2h = ((float)c2[2] - b) / g_pixelsPerUnit;	//for volume truncation later

        float hx = (c2[0] - c1[0]) * (a / lr_pixels) + c1[0];
        float hy = (c2[1] - c1[1]) * (a / lr_pixels) + c1[1];
        //Point P2 = P1.sub(P0).scale(a/d).add(P0);
        i1x = hx + (hi * (float)(c2[1] - c1[1]) / lr_pixels);
        i1y = hy - (hi * (float)(c2[0] - c1[0]) / lr_pixels);
        i2x = hx - (hi * (float)(c2[1] - c1[1]) / lr_pixels);
        i2y = hy + (hi * (float)(c2[0] - c1[0]) / lr_pixels);
        iDistance = (float)sqrt((i2y - i1y) * (i2y - i1y) + (i2x - i1x) * (i2x - i1x));
        float halfpi = (float)MATH_PI / 2.0f;
        float ia1 = (float)abs(acos(a / c1[2]));
        float ia2 = (float)abs(acos(b / c2[2]));
        //source: http://stackoverflow.com/questions/3349125/circle-circle-intersection-points

        gr = (float)c1[2] / (float)g_pixelsPerUnit;
        sr = (float)c2[2] / (float)g_pixelsPerUnit;
        gv = ((float)MATH_PI * 4.0f * gr * gr * gr) / 3.0f;
        gv -= ((float)MATH_PI * c1h * (3.0f * rdib * rdib + c1h * c1h)) / 6.0f;
        //InformationPanel.growingVolume.setText(to_string(gv));

        sv = ((float)MATH_PI * 4.0f * sr * sr * sr) / 3.0f;
        sv -= ((float)MATH_PI * c2h * (3.0f * rdib * rdib + c2h * c2h)) / 6.0f;
        //InformationPanel.shrinkingVolume.setText(to_string(sv));

        ///*DEBUG*/ cout << gr << "\t" << rdib << "\t" << c1h << endl;
        ///*DEBUG*/ cout << sr << "\t" << rdib << "\t" << c2h << endl;

        float v = ((float)MATH_PI * 4.0f * big * big * big) / 3.0f;
        v += ((float)MATH_PI * 4.0f * small * small * small) / 3.0f;
        v -= ((float)MATH_PI * c1h * (3.0f * rdib * rdib + c1h * c1h)) / 6.0f;	//subtract DIB overlap
        v -= ((float)MATH_PI * c2h * (3.0f * rdib * rdib + c2h * c2h)) / 6.0f;
        tv = v;
        //InformationPanel.volumeLabel.setText(to_string(v));
    }
    else {			//circle-surface intersection
        if (c1[2] == c2[2]) return false;

        //average center; contact angle requires concentric circles
        float cX = (c1[0] + c2[0]) / 2.0f; float cY = (c1[1] + c2[1]) / 2.0f;
        float y = (float)sqrt((big * big) - (small * small));
        float yp = -1.0f * (small / y);
        float theta = (float)abs(atan(yp));
        //InformationPanel.contactAngle.setText(to_string(180.0f * theta / (float)MATH_PI));

        //dome & volume calculations
        float h_dome = big - y;
        float v_dome = ((float)MATH_PI * h_dome * (3.0f * small * small + h_dome * h_dome)) / 6.0f;
        float v = ((float)MATH_PI * 4.0f * big * big * big) / 3.0f;
        v -= v_dome;
        //InformationPanel.growingVolume.setText(to_string(v));
        //InformationPanel.shrinkingVolume.setText("N/A");
        //InformationPanel.volumeLabel.setText(to_string(v));
        tv = v;
    }


    g_outfile << timeStamp << "," << gr << "," << gv << "," << sr << "," << sv << "," << tv << "," << dibRadius << "," << contactAngle << "," << rDistance << ",\n";

    cout << "\nTime Stamp\tDroplet 1 Radius\tDroplet 1 Volume\tDroplet 2 Radius\tDroplet 2 Volume\tTotal Volume\tDIB Radius\tContact Angle\tRadial Distance\n";
    cout << timeStamp << "\t" << gr << "\t" << gv << "\t" << sr << "\t" << sv << "\t" << tv << "\t" << dibRadius << "\t" << contactAngle << "\t" << rDistance << "\n";

    return true;
}

void showImageFromFile(string fullImagePath) {
    // Read the image file
    cv::Mat image = cv::imread(fullImagePath);
    // Check for failure
    if (image.empty())
    {
        cout << "Image Not Found!!!" << endl;
        cin.get(); //wait for any key press
        return;
    }
    // Show our image inside a window.
    cv::imshow("Image Window Name here", image);

    // Wait for any keystroke in the window
    cv::waitKey(0);
}

void showVideoFromFile(string fullPath) {
    // Create a VideoCapture object and open the input file
    // If the input is the web camera, pass 0 instead of the video file name
    cv::VideoCapture cap(fullPath);

    // Check if camera opened successfully
    if (!cap.isOpened()) {
        cout << "Error opening video stream or file" << endl;
        return;
    }

    bool validFrame = true;

    //int scaleFrameStartX, scaleFrameStartY, scaleFrameEndX, scaleFrameEndY;

    //int scaleLineLength = 0;
    //int scaleLineStartR, scaleLineStartC, scaleLineEndC;

    Vec3i c1, c2, c1Last, c2Last;
    Vec4i separatorLine, separatorLineLast;

    while (1) {

        cv::Mat frame;
        // Capture frame-by-frame
        cap >> frame;
        g_currentFrame++;

        // Find scale line and set scale if not set already
        if (!g_scaleSet) {

            g_scaleFrameStartX = (float)frame.size().width * 0.5f;
            g_scaleFrameStartY = (float)frame.size().height * 0.8f;
            g_scaleFrameEndX = ((float)frame.size().width * 0.5f) - 1;
            g_scaleFrameEndY = ((float)frame.size().height * 0.2f) - 1;
            Mat scaleFrame = frame(Rect(g_scaleFrameStartX, g_scaleFrameStartY, g_scaleFrameEndX, g_scaleFrameEndY));
            cv::cvtColor(scaleFrame, scaleFrame, COLOR_BGR2GRAY);
            cv::threshold(scaleFrame, scaleFrame, 100, 255, THRESH_OTSU);
            bitwise_not(scaleFrame, scaleFrame);

            for (int r = 0; r < scaleFrame.rows; r++) {
                bool foundblack = false;
                int thisBlackLineLength = 0;
                int thisLineStart = 0;
                for (int c = 0; c < scaleFrame.cols; c++) {
                    if (!foundblack) {
                        if (scaleFrame.at<uchar>(r, c) == 0) {
                            thisLineStart = c;
                            thisBlackLineLength++;
                            foundblack = true;
                        }
                    }
                    else if (foundblack) {
                        if (scaleFrame.at<uchar>(r, c) != 0) {
                            break;
                        }
                        thisBlackLineLength++;
                        if (thisBlackLineLength > g_scaleLineLength) {
                            g_scaleLineLength = thisBlackLineLength;
                            g_scaleLineStartR = r;
                            g_scaleLineStartC = thisLineStart;
                            g_scaleLineEndC = c;
                        }
                    }
                }
            }

            g_scalePixels = g_scaleLineEndC - g_scaleLineStartC;
            g_pixelsPerUnit = (float)g_scalePixels / (float)g_scaleUnits;
            cv::line(scaleFrame, Point(g_scaleLineStartC, g_scaleLineStartR), Point(g_scaleLineEndC, g_scaleLineStartR), Scalar(0, 0, 255), LINE_AA);

            //imshow("Scale Frame", scaleFrame);
            g_scaleSet = true;
        }

        //skip frames
        if (g_currentFrame % g_frameskip != 0 && validFrame) continue;

        // If the frame is empty, break immediately
        if (frame.empty())
            break;

        Mat originalFrame = frame;
        Mat dst = Mat::zeros(frame.rows, frame.cols, CV_8U);

        vector<cv::Vec3f> circles;
        //cv::Mat circles = cv::Mat();
        cv::Scalar color = cv::Scalar(255, 0, 0);
        cv::cvtColor(frame, frame, cv::COLOR_RGBA2GRAY, 0);
        // crop timestamp and scale bar from bottom of video so Otsu works
        Mat measureFrame = frame(Rect(0, 0, (float)frame.size().width - 1.0f, ((float)frame.size().height * 0.875f) - 1));
        if (g_threshold == -1) {
            double threshold = cv::threshold(measureFrame, measureFrame, 100, 255, cv::THRESH_OTSU);
        }
        else { //25
            double threshold = cv::threshold(measureFrame, measureFrame, g_threshold, 255, THRESH_BINARY);
        }
        cv::HoughCircles(measureFrame, circles, cv::HOUGH_GRADIENT, 3,
            10, 100, 50,
            measureFrame.rows / 10, measureFrame.rows / 4);
        cv::cvtColor(frame, frame, cv::COLOR_GRAY2RGB, 0);
        validFrame = true;
        // draw 2 best circles
        // Take 2 best circles from Hough array and figure out which is which
        if (circles.size() < 2) continue;
        if (g_firstframe) {
            c1 = circles[0];
            c2 = circles[1];
            c1Last = c1;
            c2Last = c2;

            //TODO: if this frame is invalid skip it or something
            if (circlesInsideEachOther(c1, c2)) {
                validFrame = false;
            }

            if (validFrame) {

                // Separator line doesn't really work
                //// create line perpendicular to line connecting circle centers
                //separatorLine = getSeparatorLine(Point(c1[0], c1[1]), Point(c2[0], c2[1]));

                //// make sure c1 is always above the separator line in point slope form
                //if ((c1[1] - separatorLine[3]) > (getSlope(separatorLine) * (c1[0] - separatorLine[2]))) {
                //    Vec3i c3 = c2; c2 = c1; c1 = c3;
                //}
                 
                // make sure c2 is rightmost circle
                if (c1[0] > c2[0]) {
                    Vec3i c3 = c2; c2 = c1; c1 = c3;
                }
                process(c1, c2);

                g_firstframe = false;
            }
        }
        else {
            if (circlesInsideEachOther(circles[0], circles[1])) {
                validFrame = false;
            }

            if (validFrame) {
                c1Last = c1;
                c2Last = c2;

                c1 = circles[0];
                c2 = circles[1];
                separatorLineLast = separatorLine;
                separatorLine = getSeparatorLine(Point(c1[0], c1[1]), Point(c2[0], c2[1]));

                // Separator line doesn't really work
                //// make sure c1 is always above the separator line in point slope form
                //if ((c1[1] - separatorLineLast[3]) > (getSlope(separatorLineLast) * (c1[0] - separatorLineLast[2]))) {
                //    Vec3i c3 = c2; c2 = c1; c1 = c3;
                //}

                //// swap circles based on distance from circles in previous frame
                //if (sqDistance(Point(c1[0], c1[1]), Point(c1Last[0], c1Last[1])) > sqDistance(Point(c2[0], c2[1]), Point(c1Last[0], c1Last[1]))) {
                //    Vec3i c3 = c2; c2 = c1; c1 = c3;
                //}

                // make sure c2 is rightmost circle
                if (c1[0] > c2[0]) {
                    Vec3i c3 = c2; c2 = c1; c1 = c3;
                }
                process(c1, c2);
            }
        }

        // draw the circle centers
        cv::circle(frame, Point(c1[0], c1[1]), 3, Scalar(0, 255, 0), -1, 8, 0);
        cv::circle(originalFrame, Point(c1[0], c1[1]), 3, Scalar(0, 255, 0), -1, 8, 0);
        cv::circle(frame, Point(c2[0], c2[1]), 3, Scalar(0, 255, 0), -1, 8, 0);
        cv::circle(originalFrame, Point(c2[0], c2[1]), 3, Scalar(0, 255, 0), -1, 8, 0);
        // draw the circle outlines
        cv::circle(frame, Point(c1[0], c1[1]), c1[2], Scalar(0, 0, 255), 3, 8, 0);
        cv::circle(originalFrame, Point(c1[0], c1[1]), c1[2], Scalar(0, 0, 255), 3, 8, 0);
        cv::circle(frame, Point(c2[0], c2[1]), c2[2], Scalar(255, 0, 0), 3, 8, 0);
        cv::circle(originalFrame, Point(c2[0], c2[1]), c2[2], Scalar(255, 0, 0), 3, 8, 0);


        //draw separator line
        cv::line(frame, Point(separatorLine[0], separatorLine[1]), Point(separatorLine[2], separatorLine[3]), Scalar(0, 255, 0), LINE_AA);
        cv::line(frame, Point(c1[0], c1[1]), Point(c2[0], c2[1]), Scalar(0, 255, 0), LINE_AA);

        //draw red X if the frame is invalid
        if (!validFrame) {
            cv::line(frame, Point(0, 0), Point(frame.cols, frame.rows), Scalar(0, 0, 255), LINE_AA);
            cv::line(frame, Point(frame.cols, 0), Point(0, frame.rows), Scalar(0, 0, 255), LINE_AA);
            cv::line(originalFrame, Point(0, 0), Point(originalFrame.cols, originalFrame.rows), Scalar(0, 0, 255), LINE_AA);
            cv::line(originalFrame, Point(originalFrame.cols, 0), Point(0, originalFrame.rows), Scalar(0, 0, 255), LINE_AA);
        }

        // draw scale line in red
        cv::line(frame, Point(g_scaleFrameStartX + g_scaleLineStartC, g_scaleFrameStartY + g_scaleLineStartR), Point(g_scaleFrameStartX + g_scaleLineEndC, g_scaleFrameStartY + g_scaleLineStartR), Scalar(0, 0, 255), LINE_AA);
        cv::line(originalFrame, Point(g_scaleFrameStartX + g_scaleLineStartC, g_scaleFrameStartY + g_scaleLineStartR), Point(g_scaleFrameStartX + g_scaleLineEndC, g_scaleFrameStartY + g_scaleLineStartR), Scalar(0, 0, 255), LINE_AA);

        // Display the resulting frame
        int window_width = 800;
        int window_height = 800;
        cv::Mat resized_frame;

        //change first param to frame for the working frame, originalFrame for export
        cv::resize(frame, resized_frame, cv::Size(window_width, window_height), cv::INTER_LINEAR);
        cv::imshow("Frame", resized_frame);

        // Press  ESC on keyboard to exit
        char c = (char)cv::waitKey(25);
        if (c == 27)
            break;
    }

    // When everything done, release the video capture object
    cap.release();

    // Closes all the frames
    cv::destroyAllWindows();
}

int main(int argc, char** argv)
{
    if (argc < 2) return -1;
    string fname = argv[1];

    if (argc >= 3) g_frameskip = atoi(argv[2]);
    if (argc >= 4) g_threshold = atoi(argv[3]);

    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-o" || arg == "--olympus") {

            g_scaleFrameStartX = 1675;
            g_scaleFrameStartY = 1122;
            g_scaleFrameEndX = 1905;
            g_scaleFrameEndY = 1186;

            g_scaleLineLength = 216;
            g_scaleLineStartR = 1127;
            g_scaleLineStartC = 1684;
            g_scaleLineEndC = 1900;

            g_scalePixels = g_scaleLineEndC - g_scaleLineStartC;
            g_pixelsPerUnit = (float)g_scalePixels / (float)g_scaleUnits;

            g_scaleSet = true;
        }
    }

    VideoCapture video(fname);
    g_framesPerSecond = (int)video.get(CAP_PROP_FPS);

    string outfname = fname.substr(0, fname.find_last_of('.')) + ".csv";

    g_outfile.open(outfname, ios::out);
    g_outfile << "Time Stamp,Droplet 1 Radius,Droplet 1 Volume,Droplet 2 Radius,Droplet 2 Volume,Total Volume,DIB Radius,Contact Angle,Radial Distance,\n";

    showVideoFromFile(fname);
    //showVideoFromFile("C:\\Users\\geoff\\source\\repos\\DropletShapeAnalysis\\DropletShapeAnalysis\\Test Videos\\WP 30C 4POPC 1CHOL 187mosm sqe016.mp4");

    g_outfile.close();

    return 0;
}


float sqDistance(Point p1, Point p2)
{
    return (float)((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

// create line perpendicular to line connecting circle centers
Vec4i getSeparatorLine(Point p1, Point p2)
{
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double mag = sqrt(dx * dx + dy * dy);
    dx /= mag;
    dy /= mag;
    Point2i mid = Point2i((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
    int x3 = mid.x + (100 * dy);
    int y3 = mid.y - (100 * dx);
    int x4 = mid.x - (100 * dy);
    int y4 = mid.y + (100 * dx);

    return Vec4i(x3, y3, x4, y4);
}

// return slope of a line or return a large number if the line is vertical
float getSlope(Vec4i line)
{
    if (line[0] == line[2]) return INT_MAX;
    return (line[3] - line[1]) / (line[2] - line[0]);
}

bool circlesInsideEachOther(Vec3i c1, Vec3i c2)
{
    //make c2 smaller circle
    if (c2[2] > c1[2]) {
        Vec3i c3 = c2; c2 = c1; c1 = c3;
    }
    int distSq = ((c1[0] - c2[0]) * (c1[0] - c2[0])) + ((c1[1] - c2[1]) * (c1[1] - c2[1]));
    if (distSq + (c2[2] * c2[2]) <= (c1[2] * c1[2])) return true;
    return false;
}
