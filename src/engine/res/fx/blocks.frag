in vec3 col;
in vec2 coord;
in float z;
uniform sampler2D atlas;
uniform vec3 fog_color=vec3(0.8,0.8,0.8);
layout(location=0) out vec4 frag_color0;
void main(){
	frag_color0=vec4(mix(col*texture(atlas,coord.xy).rgb, fog_color, 0.9*z),1.0);
}
