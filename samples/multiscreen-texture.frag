#version 120 

varying vec2 v_TexCoord; 

uniform sampler2D u_Texture; 
uniform float u_Alpha; 

void main() 
{
    gl_FragColor = texture2D(u_Texture, v_TexCoord); 
    gl_FragColor.a *= u_Alpha; 
}
