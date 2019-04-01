layout(location = 0) in uint L1;
layout(location = 1) in uint L2;
layout(location = 2) in uint L3;
out vec4 color;
out vec4 albedox;
uniform int x0,y0;
uniform float px, py;
uniform float gi_radius; uniform vec2 gi_scale;
uniform vec3 sun_dir;

uniform sampler2D SM;
uniform mat4x4 light_matrix;
#define EMPTY 0
#define AIR 1
#define WATER 2
#define SAND 3
#define GRASS 4
#define STONE 5
#define DIRT 6
#define SNOW 7
#define SANDSTONE 8
vec3 block_getrgb(int bc) {
	float r,g,b;
	if (bc < WATER) {
		r = 0.1;
		g = 0.1;
		b = 0.2;
	}
	else if(bc==SAND){
		r = 1.0;
		g = 0.9;
		b = 0.5;
	}
	else if (bc == SANDSTONE) {
		r = 0.8;
		g = 0.7;
		b = 0.3;
	}
	else if (bc == STONE) {
		r = 0.4;
		g = 0.5;
		b = 0.3;
	}
	else if (bc == DIRT) {
		r = 0.6;
		g = 0.5;
		b = 0.3;
	}
	else if (bc == SNOW) {
		r = 0.9;
		g = 0.9;
		b = 0.9;
	}
	else if (bc == WATER) {
		r = 1.0;
		g = 1.0;
		b = 1.0;
	}

	else if (bc == GRASS) {
		r = 0.6;
		g = 0.7;
		b = 0.3;
	}
	else {
		r = 1; b = 1; g = 1;
	}
	return vec3(r,g,b);
}
void main(){
				
				uint L1x=L1&0xffu;
				uint L1y=(L1>>8u)&0xffu;
				uint L1z=(L1>>16u)&0xffu;
				uint L1w=(L1>>24u)&0xffu;
				
				uint L2x=L2&0xffu;
				uint L2y=(L2>>8u)&0xffu;
				uint L2z=(L2>>16u)&0xffu;
				uint L2w=(L2>>24u)&0xffu;

				uint L3x=L3&0xffu;
				uint L3y=(L3>>8u)&0xffu;
				uint L3w=(L3>>24u)&0xffu;
				uint xydiff=(L3>>16u)&0xffu;
				ivec3 positioni=ivec3(int(xydiff)/16+x0,int(xydiff)%16+y0,int(L3y)); 
								
				vec3 position=vec3(float(positioni.x),float(positioni.y),float(positioni.z));
				vec3 xcol=vec3(float(L1x),float(L1y),float(L1z))/255.0;
				
				
				
			albedox=vec4(block_getrgb(int(L3w)),1.0);
			float N=gi_radius*2.0/gi_scale.x;
			float Nz=255.0/gi_scale.y;
			vec3 xyz=position-vec3(px,py,0);
			xyz=xyz/gi_scale.xxy;
			color=vec4(xcol.rgb,1.0);
/*			vec4 lpos= light_matrix*vec4(position,1.0);
			lpos/=lpos.w;
			albedox.w=float(texture(SM, lpos.rg*0.5+0.5).r*2.0-1.0>=lpos.z);*/
			gl_Position=vec4(((xyz.x)/N+(xyz.y)/(N*N)), xyz.z/Nz,0,1.0);
			gl_PointSize=3;
}
