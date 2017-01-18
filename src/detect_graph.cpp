#include <iostream>
#include <cmath>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/photo.hpp"

#include "edge_detection/edge_detection.hpp"
#include "vertex_detection/vertex_detection.hpp"
#include "tikz_generation/graph.hpp"

using namespace cv;
using namespace std;

int main(int argc, char *argv[])
{
    if (argv[1] == NULL){
        cout << argc << endl; 
        cout << "Pass image as argument." << endl;
        return 1;
    }
    Mat img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
    Mat img_color = imread(argv[1]);

    //fastNlMeansDenoising(img, img, 3, 7, 21);
    //equalizeHist(img,img);

    vertex_detection vd;
    vector<vertex> detected_vertices;
    vector<Rect> bboxes;


    detected_vertices = vd.v_detect(img_color);    


    uint vertex_count = detected_vertices.size(); // not quite right ....
    cout << "# detected vertices: " << vertex_count << endl;


    vector<vector<bool>> A(vertex_count, vector<bool>(vertex_count, false));
    edge_detection ed(img, A, detected_vertices, vd.bboxes, vd.avg_vertex_radius);


    ed.trace_all_edges(detected_vertices);
    
    graph_layout gl = graph_layout(img, detected_vertices, ed.A);
    gl.generate_file("asd.tex");

    waitKey(0);

    return 0;
}
