in vec3 col;
in vec2 coord;
in float z;

uniform sampler2D atlas;
uniform vec3 fog_color;
uniform vec3 amb;
layout(location=0) out vec4 frag_color0;
void main(){
	frag_color0=vec4(mix((col+amb)*texture(atlas,coord.xy).rgb,fog_color,max(z-0.8,0.0)), 1.0);
}
