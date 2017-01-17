#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/features2d/features2d.hpp"
#include <iostream>
#include <cmath>

using namespace cv;
using namespace std;

typedef Point vertex;

class vertex_detection{

public:
    double avg_vertex_radius; 
    vector<Rect> bboxes;


private:
    bool valid_vertex(cv::Rect rect){
        double ratio = rect.width/(double)rect.height;
        double acceptable_ratio = 1/1.5;
        if (!(ratio < acceptable_ratio or ratio > 1/acceptable_ratio)){
            return true;
        }
        return false;
    }

private:
    bool bbox_intersect(Rect a, Rect b){
        bool a_in_b = a.x >= b.x && a.x <= b.x + b.width && a.y > b.y && a.y < b.y + b.height;
        if(a_in_b){
            return true;
        }
        return false;
    }

private:
    bool bbox_overlap(Rect a, Rect b){
        return a.x < b.x + b.width &&
               a.x + a.width > b.x &&
               a.y < b.y + b.height &&
               a.y + a.height > b.y;
    }

private: 
    void discard_box(vector<Rect>& bboxes, size_t index){
        bboxes.erase(remove(bboxes.begin(), bboxes.end(), bboxes[index]), bboxes.end()); 
    }

private:
    void discard_smaller_bbox(vector<Rect>& bboxes, size_t i, size_t j){
        Rect smaller_box;
        if((bboxes[i].width * bboxes[i].height) <= (bboxes[j].width * bboxes[j].height)){
            smaller_box = bboxes[i];
        }else{
            smaller_box = bboxes[j];
        }
        bboxes.erase(remove(bboxes.begin(), bboxes.end(), smaller_box), bboxes.end()); 
    }

private:
    void discard_bboxes_inside_other(vector<Rect>& bboxes){
        for(size_t i = 0; i < bboxes.size(); i++){
            for(size_t j = i+1; j < bboxes.size(); j++){
                if(bbox_intersect(bboxes[i], bboxes[j]) || bbox_intersect(bboxes[j], bboxes[i])){ 
                    discard_smaller_bbox(bboxes, i, j);
                }    
            }
        }
    }

private:
    void discard_close_bboxes(Mat img, vector<Rect>& bboxes){
        double threshold = 25.0;
        Point upper_left(0.0, 0.0);
        Point lower_left(img.rows, 0.0);

        for(size_t i = 0; i < bboxes.size(); i++){
            for(size_t j = i+1; j < bboxes.size(); j++){
        
                Point top_left_i = bboxes[i].tl();
                Point bottom_right_i = bboxes[i].br();

                Point top_left_j = bboxes[j].tl();
                Point bottom_right_j = bboxes[j].br();

                if(norm(top_left_i - top_left_j) < threshold){
                    discard_smaller_bbox(bboxes, i, j);
                }
                else if(norm(bottom_right_i - bottom_right_j) < threshold){
                    discard_smaller_bbox(bboxes, i, j);
                }
                if(norm(top_left_i-upper_left) < 20){
                    discard_box(bboxes, i);
                }
                if(top_left_i.y < 15){
                    discard_box(bboxes, i);
                }
                if(top_left_i.x < 15){
                    discard_box(bboxes, i);
                }
            }
        }
    }

private:
    //  if (RectA.Left < RectB.Right && RectA.Right > RectB.Left &&
    //   RectA.Top < RectB.Bottom && RectA.Bottom > RectB.Top ) 
    void discard_overlapping_bboxes(Mat img, vector<Rect>& bboxes){
        for(size_t i = 0; i < bboxes.size(); i++){
            for(size_t j = 0; j < bboxes.size(); j++){
                if(i == j)
                    continue;
                if(bbox_overlap(bboxes[i], bboxes[j])){
                    discard_smaller_bbox(bboxes, i, j);                
                }
            }
        }
 
    }

public:
    vector<Rect> compute_MSER(Mat img){

        uint H = img.rows;
        uint W = img.cols;

        double min_vertex_size = (H*W)/5000.0;
        double max_vertex_size = (H*W)/200.0;
        cout << "min V size:  " << min_vertex_size << endl;
        cout << "max V size:  " << max_vertex_size << endl;


        int _delta = 5;                   //     it compares (size_i - size_ -delta)/size_i-delta
        int _min_area = min_vertex_size;  //     prune the area which smaller than minArea
        int _max_area = max_vertex_size;  //     prune the area which bigger than maxArea 
        double _max_variation = .25;      //     prune the area have simliar size to its children
        double _min_diversity = .2;       //     for color image, trace back to cut off mser with diversity less than min_diversity
        int _max_evolution = 30;          //     for color image, the evolution steps
        double _area_threshold = 1.01;    //     for color image, the area threshold to cause re-initialize
        double _min_margin = .003;       //     for color image, ignore too small margin
        int _edge_blur_size = 3;          //     for color image, the aperture size for edge blur

        Ptr<MSER> ms = MSER::create(_delta, _min_area, _max_area, _max_variation, 
                _min_diversity, _max_evolution, _area_threshold, 
                _min_margin, _edge_blur_size);
        vector<vector<Point> > regions;
        vector<Rect> mser_bbox;
        ms->detectRegions(img, regions, mser_bbox);

        return mser_bbox;

    }


public:
    vector<vertex> v_detect(Mat img_color){
        Mat img_gray;
        cvtColor(img_color, img_gray, CV_BGR2GRAY);


        vector<vertex> detected_vertices;
        vector<double> bbox_edge_length;

        vector<Rect> mser_bbox       = compute_MSER(img_color);
        vector<Rect> mser_bbox_gray  = compute_MSER(img_gray);

        mser_bbox.insert(mser_bbox.end(), mser_bbox_gray.begin(), mser_bbox_gray.end());



        //discard_bboxes_inside_other(mser_bbox);
        discard_close_bboxes(img_color, mser_bbox);
        //discard_overlapping_bboxes(img_color, mser_bbox);

        //groupRectangles(mser_bbox, 1);

        for (int i = 0; i < mser_bbox.size(); i++){
            if(valid_vertex(mser_bbox[i])){
                rectangle(img_color, mser_bbox[i], CV_RGB(0, 50, 255));
                Point top_left = mser_bbox[i].tl();
                Point bottom_right = mser_bbox[i].br();
                double vertex_x = (top_left.x + bottom_right.x)/2.0;
                double vertex_y = (top_left.y + bottom_right.y)/2.0;
                double size_v = max(abs(top_left.x - bottom_right.x), abs(top_left.y - bottom_right.y));
                vertex v(vertex_x, vertex_y);
                detected_vertices.push_back(v);
                
                bbox_edge_length.push_back(size_v/2);
                this->bboxes.push_back(mser_bbox[i]);
            }   
        }
        this->avg_vertex_radius = accumulate(bbox_edge_length.begin(), bbox_edge_length.end(), 0.0)/bbox_edge_length.size();
        

        imshow("mser", img_color);
        return detected_vertices;
    }


};
