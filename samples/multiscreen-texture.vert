#version 120 

attribute vec2 a_Position; 
attribute vec2 a_TexCoord; 

varying vec2 v_TexCoord; 

uniform float u_Depth; 
uniform mat4 u_ModelMat; 
uniform mat4 u_ViewMat; 

void main() 
{
    gl_Position = u_ViewMat * u_ModelMat * vec4(a_Position, u_Depth, 1.0); 
    v_TexCoord = a_TexCoord; 
}
