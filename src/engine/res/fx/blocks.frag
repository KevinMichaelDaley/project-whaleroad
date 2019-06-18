
in vec3 col;
in vec2 coord;
in float z;
in float zw;

uniform sampler2D atlas;
uniform vec3 fog_color;
layout(location=0) out vec4 frag_color0;
void main(){
	frag_color0=vec4(col*texture(atlas,coord.xy).rgb, 1.0);
}
