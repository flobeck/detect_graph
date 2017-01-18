#include <string>
#include <iostream>
#include <fstream>
#include "opencv2/features2d/features2d.hpp"

using namespace std;
using namespace cv;

class graph_layout{

public: 
    uint W, H;
    vector<Point> positions;

    uint tikzpicture_W = 15;
    uint tikzpicture_H = 15;
    vector<vector<bool>> A; 

    graph_layout(Mat src, vector<Point> positions, vector<vector<bool>> A){
        this->W = src.cols;
        this->H = src.rows;
        this->positions = positions;
        this->A = A;
    }


private:
    Point vertex_position_tikz(Point v){
        double ratio_x = v.x /(double)this->W;
        double ratio_y = v.y /(double)this->H;

        //cout << ratio_x << endl;

        uint tikzpicture_pos_x = (uint)(ratio_x*(this->tikzpicture_W-5));
        uint tikzpicture_pos_y = (uint)(ratio_y*(this->tikzpicture_W-5));

        Point pos(tikzpicture_pos_x, this->tikzpicture_H - tikzpicture_pos_y);

        return pos;
    }

private:
    bool replace(string& str, const string& from, const string& to) {
        size_t start_pos = str.find(from);
        if(start_pos == string::npos){
            return false;
        }
        str.replace(start_pos, from.length(), to);
        return true;
}


public: 
    void generate_file(string filename){
        string header = "\\documentclass{article} \n"
                        "\\usepackage{tikz} \n"
                        "\\begin{document}  \n"
                        "\\begin{tikzpicture}[place/.style={circle,auto,draw=black,thick}] \n \n";

        string footer = "\\end{tikzpicture} \n"
                        "\\end{document}";

        ofstream tex_file(filename);
        tex_file << header;

        string node_str;

        for(size_t i = 0; i < this->positions.size(); i++){            
            node_str = "\\node (N) at (P) [place] {};\n";

            replace(node_str, "N", to_string(i));

            Point tikz_v_pos = vertex_position_tikz(this->positions[i]);
            replace(node_str, "P", to_string(tikz_v_pos.x) + ", " + to_string(tikz_v_pos.y));

            tex_file << node_str;
        }
        
        tex_file << "\\path[draw, thick] \n";

        for(size_t i = 0; i < this->A.size(); i++){
            for(size_t j = 0; j < this->A[i].size(); j++){
                if(this->A[i][j]){
                    tex_file << "(" << to_string(i) << ") edge node {}" << "(" << to_string(j) << ")";
                }
            }        
        }
        tex_file << ";";   


        tex_file << footer;
        tex_file.close();
        
    }
    
};
