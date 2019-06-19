
in vec3 col;
in vec2 coord;
in vec2 zw;
in float shadow;

uniform sampler2D atlas;
uniform vec3 fog_color;
layout(location=0) out vec4 frag_color0;
void main(){
   
	frag_color0=vec4(mix(texture(atlas,coord.xy).rgb*(shadow+1)*(col+0.1)*0.5,fog_color,max(0,zw.x/zw.y*0.5+0.5)*0.25), 1.0);
	//frag_color0=vec4(col.r/2,col.r/2,col.r/2,1.0);
}
