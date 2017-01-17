#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "../feature/image_features.hpp"
#include <iostream>
#include <cmath>

using namespace cv;
using namespace std;

typedef Point vertex;


Mat trace_img, copy_img;


class edge_detection{

public:
    Mat src_gray;
    vector<vector<bool>> A;  //adjacency matrix
    vector<vertex> detected_vertices;
    vector<Rect> bboxes;
    double avg_vertex_radius;


    edge_detection(Mat src_gray, 
        vector<vector<bool>> A, 
        vector<vertex> detected_vertices, 
        vector<Rect> bboxes, 
        double avg_vertex_radius)
    { 
        this->src_gray = src_gray;
        this->A = A;
        this->detected_vertices = detected_vertices;
        this->bboxes = bboxes;
        this->avg_vertex_radius = avg_vertex_radius;

        trace_img = src_gray.clone();
        copy_img = src_gray.clone();
    }


    void place_circles(Mat img, vector<Point> points){
        for (Point& p : points){
            circle(img, p, 1, Scalar(250), 5, 8, 0);
        }
        imshow("trace_img", img);
    }

    vector<Point> circles_coordinates(Point circle_center, double radius){
        vector<Point> points_on_circle;
        double steps = 2.0*M_PI/1000;

        for (double angle = 0.0; angle < 2.0*M_PI; angle+=steps){
            int circle_point_x = floor(circle_center.x + radius * cos(angle));
            int circle_point_y = floor(circle_center.y + radius * sin(angle));
            Point p(circle_point_x, circle_point_y);
            points_on_circle.push_back(p);
        }
        return points_on_circle;
    }

    vector<Point> sector_coordinates(Point circle_center, Point direction_vec){
        vector<Point> points_on_sector;
        double steps = 1/100.0;

        Point point_on_direction_vec = circle_center + direction_vec;
        double radius = 10; //norm(direction_vec);
        double r = atan2(point_on_direction_vec.y - circle_center.y, point_on_direction_vec.x - circle_center.x);


        for (double angle = r-M_PI/3; angle < r+M_PI/3; angle+=steps){
            int circle_point_x = floor(circle_center.x + radius * cos(angle));
            int circle_point_y = floor(circle_center.y + radius * sin(angle));
            Point p(circle_point_x, circle_point_y);
            points_on_sector.push_back(p);
        }    

        return points_on_sector;
    }

    vector<xy_value> pixel_values(vector<Point> points){
        vector<xy_value> pixel_values;

        for (size_t i = 0; i < points.size(); i++){
            if(check_coordinate(this->src_gray, points[i])){

                uint pixel_value = pixel_value_grayscale(this->src_gray, points[i]);
                xy_value xyv = xy_value(points[i], pixel_value);

                pixel_values.push_back(xyv);
            }
        }
        return pixel_values;
    }

    double dist(Point a, Point b){
        return norm(a-b);
    }

    /*
       maybe just check deviation of points_on_circle....
       TODO: if std dev very low => no edges
    */
    vector<Point> get_starting_points(vertex start_vertex){    
        vector<Point> starting_points, information_points;

        bool bright_image = bright_background(this->src_gray);
        double neighbouring_pts_threshold = 10;

        vector<Point> points_on_circle = circles_coordinates(start_vertex, 2*this->avg_vertex_radius);
        vector<xy_value> cpv = pixel_values(points_on_circle);
        //place_circles(copy_img, points_on_circle);


        cout << "std dev:  " << std_dev(cpv) << endl;
        double m = mean(cpv);
        double s = std_dev(cpv);
        double s_factor = 1.0;

        if(std_dev(cpv) < 15.0){
            s_factor = 1.5;
        }

        size_t P = cpv.size();
        for(size_t i = 0; i < P; i++){
            if(check_coordinate(this->src_gray, cpv[i].xy) &&
                    contains_information_std_dev(cpv[i].value, m, s_factor*s)){
                information_points.push_back(cpv[i].xy); 
            }
        }

        P = information_points.size();
        size_t i = 0;
        size_t group_begin = 0;

        while(i < P){
            if(dist(information_points[(i-1)%P], information_points[i]) < neighbouring_pts_threshold){
                if(group_begin == 0){
                    group_begin = i;
                }
            }else{
                if(group_begin != 0){
                    starting_points.push_back(information_points.at((group_begin+i)/2));
                }
                starting_points.push_back(information_points.at(i));
                group_begin = 0;
            }
            i++;
        }

        place_circles(copy_img, starting_points);
        return starting_points;
    }


    Point next_trace_point(vector<Point> candidates, Point point_on_sector, bool bright_image){
        Point next(0.0, 0.0);
        double closest = DBL_MAX;
        vector<xy_value> pv = pixel_values(candidates);

        double m = mean(pv);
        double s = std_dev(pv);
        double s_factor = 1.0;

        if(std_dev(pv) < 15.0){
            s_factor = 1.5;
        }

        Point ul(0.0,0.0);

        for (size_t i = 0; i < pv.size(); i++){
            if(check_coordinate(this->src_gray, pv[i].xy)){
                if(contains_information_std_dev(pv[i].value, m, s_factor*s)){
                    double dist = norm(pv[i].xy - point_on_sector);
                    if(dist < closest){
                        closest = dist;
                        next = pv[i].xy;
                    }
                }
            }
        }
        //if (next == ul && check_coordinate(this->src_gray, candidates[highest_information(pv)])){
        //    next = candidates[highest_information(pv)];
        //}

        if (next == ul){
            next = candidates[candidates.size()/2];
        }

        return next;
    }



    vector<Point> trace_edge(uint start_idx, vertex start_vertex, Point direction_vec){
        bool bright_image = bright_background(this->src_gray);
        bool vertex_reached = false;
        vector<Point> trace_points;
        uint num_trace_points = 0;

        while(!vertex_reached){

            if(num_trace_points > 1/4.0*max(this->src_gray.rows, this->src_gray.cols)){
                vertex_reached = true;
            }

            vector<Point> sc = sector_coordinates(start_vertex, direction_vec);
            Point point_on_sector = start_vertex + direction_vec;
            Point next = next_trace_point(sc, point_on_sector, bright_image);

            if(!check_coordinate(this->src_gray, next)){
                vertex_reached = true;
            }

            for (size_t i = 0; i < this->bboxes.size(); i++){
                if(dist_pt_rect(next, this->bboxes[i]) < 15){
                    vertex_reached = true;
                    // adjacency matrix
                    A[start_idx][i] = true;
                }

            }

            trace_points.push_back(next);

            direction_vec = next - start_vertex;
            start_vertex = next;

            num_trace_points++;
        } 


        return trace_points;
    }

    void trace_edges_from_vertex(vertex start_vertex, size_t vertex_idx, vector<Point> starting_points){

        for (size_t i = 0; i < starting_points.size(); i++){

            Point p = starting_points[i];

            Point direction_vec = p - start_vertex;
            Point pointing_to = p + direction_vec;

            line(trace_img, p, pointing_to, Scalar(250), 1, 8, 0);
            vector<Point> sc = sector_coordinates(p, direction_vec);

            //place_circles(trace_img, sc);
            vector<Point> next_ = trace_edge(vertex_idx, p, direction_vec);
            //circle(trace_img, next, 2, Scalar(250), 5, 8, 0);
            place_circles(trace_img, next_);

        }
    }


    void trace_all_edges(vector<vertex> detected_vertices){
        vector<Point> starting_points;
        size_t vertex_idx = 0;

        for (vertex& v : detected_vertices){

            starting_points = get_starting_points(v);
            trace_edges_from_vertex(v, vertex_idx, starting_points);
            vertex_idx++;
        }


        imshow("trace_img", trace_img);
        imshow("copy_img", copy_img);

    }




};
