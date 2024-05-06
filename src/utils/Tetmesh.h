#include <LavaCake/Math/basics.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <LavaCake/Helpers/ABBox.h>

class Tetmesh
{
public:
    Tetmesh(std::vector<LavaCake::vec4f> vertices, std::vector<LavaCake::vec4u> indices){
        m_vertices = vertices;
        m_indices = indices;
    }


    std::vector<LavaCake::vec4f> m_vertices;
    std::vector<LavaCake::vec4u> m_indices; 
};



Tetmesh load_msh(std::string filename){
    std::ifstream infile(filename);

    std::vector<LavaCake::vec4f> vertices;
    std::vector<LavaCake::vec4u> indices; 
    auto bbox = LavaCake::Helpers::ABBox<4,float>();

    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        //int a, b;
        //if (!(iss >> a >> b)) { break; }

        std::string s;
        getline(iss, s, ' ');


        
        if(s == "v"){
            LavaCake::vec4f v;
            getline(iss, s, ' ');
            v[0] = std::stof(s);
            getline(iss, s, ' ');
            v[1] = std::stof(s);
            getline(iss, s, ' ');
            v[2] = std::stof(s);
            v[3] = 1.0f;

            vertices.push_back(v);
            bbox.addPoint(v);
        }

        if(s == "t"){
            LavaCake::vec4u i;
            getline(iss, s, ' ');
            i[0] = std::stoi(s);
            getline(iss, s, ' ');
            i[1] = std::stoi(s);
            getline(iss, s, ' ');
            i[2] = std::stoi(s);
            getline(iss, s, ' ');
            i[3] = std::stoi(s);
            indices.push_back(i);
        }
    }

    LavaCake::vec4f A = bbox.A();
    LavaCake::vec4f B = bbox.B();

    A[3] = 0;
    B[3] = 0;

    LavaCake::vec4f c = (B + A) / 2.0f;

    LavaCake::vec4f d = B-A;
    float s = fmax(fmax(d[0],d[1]),d[2]);

    for (int i = 0; i < vertices.size(); i++){

        vertices[i] = (vertices[i] - c)  / s  ;
        vertices[i][3] = 1.0f;

    }
    return Tetmesh(vertices,indices);
}   


