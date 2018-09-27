#version 150 

in vec2 v_TexCoord; 

uniform sampler2D u_Texture; 
uniform float u_Alpha; 

out vec4 f_Color; 

void main() 
{
    f_Color = texture(u_Texture, v_TexCoord); 
    f_Color.a *= u_Alpha; 
}
