#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>


class Trackball{
    public :
    glm::vec3 m_pos;
    glm::vec3 m_lookAt;
    glm::vec3 m_up;
    float m_moveSpeed;
    float m_rotationSpeed;

    Trackball(glm::vec3 pos, glm::vec3 lookAt, glm::vec3 up){
        m_pos = pos;
        m_lookAt = lookAt;
        m_up = up;
        m_moveSpeed = 1.0f;
        m_rotationSpeed = 1.0f;
    }

    void move(glm::vec3 move){
        glm::vec3 lookAt = glm::normalize(m_pos - m_lookAt);
        glm::vec3 vertical = glm::cross(lookAt, glm::normalize(m_up));

        glm::vec3 realMove = vertical * move[0] + m_up * move[1] + lookAt * move[2];

        m_pos    = m_pos    + realMove * m_moveSpeed;
        m_lookAt = m_lookAt + realMove * m_moveSpeed;
    }

    void zoom(float move){
        glm::vec3 lookAt = glm::normalize(m_pos - m_lookAt);

        glm::vec3 realMove = lookAt * move;

        m_pos = m_pos + realMove * m_moveSpeed;
    }

    void rotate(glm::vec2 mouseMove){
        glm::vec2 mouse = glm::normalize(mouseMove);
        glm::vec3 vertical = glm::cross(glm::normalize(m_pos - m_lookAt), glm::normalize(m_up));

        if(glm::length(mouseMove) < 0.001){
            return;
        }
        glm::vec3 rotationAxis = glm::normalize(vertical * mouse[1] * -1.0f + m_up * mouse[0]);

        glm::vec3 look_at = m_pos - m_lookAt;
        float look_atlength = glm::length(look_at);

        float amplitude = glm::length(mouseMove);

        glm::quat q = glm::angleAxis(amplitude * m_rotationSpeed, rotationAxis);

        glm::vec3 rotated_look = q * glm::normalize(look_at);
        glm::vec3 rotated_up   = q * glm::normalize(m_up);

        m_up  = glm::normalize(rotated_up);
        m_pos = m_lookAt + rotated_look * look_atlength;
    }

    glm::mat4 getview(){
        return glm::lookAtLH(m_pos, m_lookAt, m_up);
    }
};
