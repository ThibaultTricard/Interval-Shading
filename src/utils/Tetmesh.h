#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <algorithm>


struct SimpleAABB {
    glm::vec4 min_pt = glm::vec4(std::numeric_limits<float>::max());
    glm::vec4 max_pt = glm::vec4(std::numeric_limits<float>::lowest());

    void addPoint(const glm::vec4& p) {
        min_pt = glm::min(min_pt, p);
        max_pt = glm::max(max_pt, p);
    }

    glm::vec4 A() const { return min_pt; }
    glm::vec4 B() const { return max_pt; }
};


class Tetmesh
{
public:
    Tetmesh(std::vector<glm::vec4> vertices, std::vector<glm::uvec4> indices){
        m_vertices = vertices;
        m_indices = indices;
    }

    std::vector<glm::vec4>  m_vertices;
    std::vector<glm::uvec4> m_indices;
};



Tetmesh load_msh(std::string filename, bool normalize = true){
    std::ifstream infile(filename);

    std::vector<glm::vec4>  vertices;
    std::vector<glm::uvec4> indices;
    SimpleAABB bbox;

    std::string line;
    while (std::getline(infile, line))
    {
        std::istringstream iss(line);

        std::string s;
        getline(iss, s, ' ');

        if(s == "v"){
            glm::vec4 v;
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
            glm::uvec4 i;
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

    if(normalize){
        glm::vec4 A = bbox.A();
        glm::vec4 B = bbox.B();

        A[3] = 0;
        B[3] = 0;

        glm::vec4 c = (B + A) / 2.0f;

        glm::vec4 d = B - A;
        float s = std::max({d[0], d[1], d[2]});

        for (size_t i = 0; i < vertices.size(); i++){
            vertices[i] = (vertices[i] - c) / s;
            vertices[i][3] = 1.0f;
        }
    }

    return Tetmesh(vertices, indices);
}
