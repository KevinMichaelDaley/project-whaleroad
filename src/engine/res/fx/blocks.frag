in vec3 col;
in vec2 coord;
in float z;
uniform sampler2D atlas;
layout(location=0) out vec4 frag_color0;
void main(){
	frag_color0=lerp(col*vec4(texture(atlas,coord.xy).rgb,depth.x/depth.y), fog_color, 0.9*z);
}
