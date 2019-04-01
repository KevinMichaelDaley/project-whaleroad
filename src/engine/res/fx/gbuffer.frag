in vec4 xcolnmlx, xcol2nmly, xcol3nmlz;
in vec2 depth;
in vec2 coord;
uniform sampler2D atlas;
layout(location=0) out vec4 frag_color0;
layout(location=1) out vec4 frag_color1;
layout(location=2) out vec4 frag_color2;
layout(location=3) out vec4 frag_color3;
void main(){
	frag_color0=vec4(texture(atlas,coord.xy).rgb,depth.x/depth.y);
	frag_color1=xcolnmlx;
	frag_color2=xcol2nmly;
	frag_color3=xcol3nmlz;
}
