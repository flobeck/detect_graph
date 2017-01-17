#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/features2d/features2d.hpp"
#include <cmath>
#include <numeric>

using namespace cv;
using namespace std;

class xy_value{

public:
    Point xy;
    uint value;
    
    xy_value(Point xy, uint value){
        this->xy = xy;
        this->value = value;
    }

};

vector<uint> xyv_get_value(vector<xy_value> pixel_values){
    vector<uint> pv;
    for(xy_value& xyv : pixel_values){
        pv.push_back(xyv.value);
    }
    return pv;
}


double dist_pt_rect(Point p, Rect r){
    Point rectangle_center((r.tl().x + r.br().x)/2.0 , (r.tl().y + r.br().y)/2.0);
    double H = abs(r.tl().y - r.br().y);
    double W = abs(r.tl().x - r.br().x);

    double dx = max(abs(p.x - rectangle_center.x) - W/2.0,  0.0);
    double dy = max(abs(p.y - rectangle_center.y) - H/2.0, 0.0);
    
    return dx*dx + dy*dy;
}


bool check_coordinate(Mat src, Point coordinate){
    return (coordinate.x >= 0 && coordinate.x < src.cols && coordinate.y >= 0 && coordinate.y < src.rows);
}

uint pixel_value_grayscale(Mat src_gray, Point coordinate){
    return (int)src_gray.at<uchar>(coordinate.y, coordinate.x);
}


double mean(vector<xy_value> xyv){
    vector<uint> pixel_values = xyv_get_value(xyv);

    double sum = accumulate(pixel_values.begin(), pixel_values.end(), 0.0);
    double m = sum / pixel_values.size();

    return m;
}


double std_dev(vector<xy_value> xyv){
    vector<uint> pixel_values = xyv_get_value(xyv);

    double m = mean(xyv);

    vector<double> diff(pixel_values.size());
    transform(pixel_values.begin(), pixel_values.end(), diff.begin(), bind2nd(std::minus<double>(), m));
    double sq_sum = inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stddev = sqrt(sq_sum / pixel_values.size());

    return stddev;
}


vector<uint> pixel_value_sample(Mat src){
    RNG rng(12345);
    vector<uint> pixel_sample;
    
    //Mat src_gray;
    //cvtColor(src, src_gray, COLOR_RGB2GRAY);

    int rows = src.rows;
    int cols = src.cols;
    int samples = (int)(rows*cols)/100.0;
    int rand_row, rand_col;

    for (size_t i = 0; i < samples; i++){
        rand_row = rng.uniform(0, rows);
        rand_col = rng.uniform(0, cols);
        Point rand_pixel_coord(rand_col, rand_row);
        pixel_sample.push_back(pixel_value_grayscale(src, rand_pixel_coord));
    }
    return pixel_sample;
}


bool bright_background(Mat src){
    vector<uint> pixel_sample = pixel_value_sample(src);
    int samples = (int)(src.rows*src.cols)*1.0/100;
    int below_127 = 0;

    for (size_t i = 0; i < pixel_sample.size(); i++){
        if(pixel_sample[i] < 127){
            below_127++;
        }
    }
    return below_127 < samples/2;
}

bool contains_information(uint pixel_value, bool bright){
    if(bright){
        return pixel_value < 110;
    }
    if(!bright){
        return pixel_value > 145;
    }
    return 0;
}


bool contains_information_std_dev(uint pixel_value, double m, double s){
    return (abs((double)pixel_value - m) > s);
}

size_t highest_information(vector<uint> pv){
    return distance(begin(pv), min_element(begin(pv), end(pv)));
}
