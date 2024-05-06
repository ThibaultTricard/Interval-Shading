#include <LavaCake/Math/basics.h>
#include <LavaCake/Math/quaternion.h>


class Trackball{
    public : 
    LavaCake::vec3f m_pos;
    LavaCake::vec3f m_lookAt;
    LavaCake::vec3f m_up;
    float m_moveSpeed;
    float m_rotationSpeed;

    Trackball(LavaCake::vec3f pos, LavaCake::vec3f lookAt, LavaCake::vec3f up){
        m_pos = pos;
        m_lookAt = lookAt;
        m_up = up;
        m_moveSpeed =1.0f;
        m_rotationSpeed = 1.0f;
    }

    void move(LavaCake::vec3f move){
        LavaCake::vec3f lookAt = normalize(m_pos - m_lookAt);
        LavaCake::vec3f vertical = cross(lookAt, normalize(m_up));

        LavaCake::vec3f realMove = vertical * move[0] + m_up * move[1] + lookAt* move[2];

        m_pos = m_pos + realMove * m_moveSpeed;
        m_lookAt = m_lookAt + realMove * m_moveSpeed;
    }

    /*void rotate(vec3r delta, real magnitue){
        
    }*/



    void zoom(float move){
        LavaCake::vec3f lookAt = normalize(m_pos - m_lookAt);
        LavaCake::vec3f vertical = cross(lookAt, normalize(m_up));

        LavaCake::vec3f realMove =  lookAt * move;

        m_pos = m_pos + realMove * m_moveSpeed;
    }


    void rotate(LavaCake::vec2f mouseMove){
        using namespace LavaCake;
        
        auto mouse = normalize(mouseMove);
        LavaCake::vec3f vertical = cross(normalize(m_pos - m_lookAt), normalize(m_up));

        LavaCake::vec3f rotationAxis = normalize(vertical * mouse[1] *-1.0f + m_up * mouse[0]);

        LavaCake::vec3f look_at = m_pos - m_lookAt;
        float look_atlength = sqrt(dot(look_at,look_at));

        float amplitude = sqrt(dot(mouseMove,mouseMove));

        LavaCake::quaternion<float> q(amplitude * m_rotationSpeed ,rotationAxis);

        auto res = (q * normalize(look_at)) * inverse(q);
        auto q_up = (q * normalize(m_up)) * inverse(q);
        m_up = LavaCake::vec3f({q_up.qx.i, q_up.qy.j, q_up.qz.k});

        m_pos = m_lookAt + LavaCake::vec3f({res.qx.i, res.qy.j, res.qz.k}) * look_atlength;
    }

    LavaCake::mat4f getview(){

        LavaCake::vec3f lookAt = normalize(m_lookAt - m_pos);
        LavaCake::vec3f vertical = cross(lookAt, normalize(m_up));

        LavaCake::mat4f translation({
            1.0f,0.0f,0.0f,-m_pos[0],

            0.0f,1.0f,0.0f,-m_pos[1],

            0.0f,0.0f,1.0f,-m_pos[2],
            0.0f,0.0f,0.0f,1.0f

        });

        LavaCake::mat4f view({
            vertical[0],vertical[1],vertical[2],0.0f,

            m_up[0],m_up[1],m_up[2],0.0f,

            lookAt[0],lookAt[1],lookAt[2],0.0f,

            0.0f,0.0f,0.0f,1.0f

        });

        return  translation * view;


    }

};