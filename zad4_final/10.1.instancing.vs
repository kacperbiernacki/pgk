#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aOffset;
layout (location = 3) in vec4 quat;

out vec3 fColor;

uniform mat4 projection;
uniform mat4 view;
//uniform vec3 uniColor;

vec4 v_x;
vec4 v_y;
vec4 v_z;
vec4 v_w;
mat4 randRotation;

void main()
{
    /*
    randRotation = mat4(1,0,0,0,
                        0,1,0,0,
                        0,0,1,0,
                        0,0,0,1); // macierz jedynkowa
                        */
    fColor = vec3((aOffset.x+1)/2, (aOffset.y+1)/2, (aOffset.z+1)/2);

    v_x = vec4( 1.0f - (2.0f*(quat.y*quat.y)) - (2.0f*(quat.z*quat.z)),
                2.0f*quat.x*quat.y-2.0f*quat.z*quat.w,
                2.0f*quat.x*quat.z+2*quat.y*quat.w, 0);

    v_y = vec4( 2.0f*quat.x*quat.y+2.0f*quat.z*quat.w, 
                1-(2.0f*(quat.x*quat.x))-(2.0f*(quat.z*quat.z)),
                2.0f*quat.y*quat.z-2*quat.x*quat.w, 0);

    v_z = vec4( 2.0f*quat.x*quat.z-2.0f*quat.y*quat.w,
                2.0f*quat.y*quat.z+2.0f*quat.x*quat.w,
                1.0f-(2.0f*(quat.x*quat.x))-(2.0f*(quat.y*quat.y)), 0);

    v_w = vec4(0, 0, 0, 1);

    randRotation = mat4(v_x, v_y, v_z, v_w);

    gl_Position = randRotation * vec4(aPos, 1);
    //gl_Position = vec4(aPos, 1);
    gl_Position = vec4(aOffset, 0) + gl_Position;
    gl_Position = projection * view * gl_Position;
}