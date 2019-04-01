			layout(location = 0) in uint L1;
			layout(location = 1) in uint L2;
			layout(location = 2) in uint L3;
			layout(location = 3) in vec3 pos;
			layout(location = 4) in vec2 uv;
			layout(location = 5) in vec3 nml;
			uniform int x0,y0;

			out vec4 xcolnmlx, xcol2nmly, xcol3nmlz;
			out vec2 depth;
			out vec2 coord;
			uniform mat4x4 projection, view;
			
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
								
				vec3 position=vec3(float(positioni.x),float(positioni.y),float(positioni.z))+pos;
				vec3 xcol=vec3(float(L1x),float(L1y),float(L1z))/255.0;
				vec3 xcol2=vec3(float(L1w),float(L2x),float(L2y))/255.0;
				vec3 xcol3=vec3(float(L2z),float(L2w),float(L3x))/255.0;
				
				int which=int(L3w);
				coord=uv+vec2(float(which)/256.0,0.0);
				xcolnmlx=vec4(xcol,nml.x*0.5+0.5);
				xcol2nmly=vec4(xcol2,nml.y*0.5+0.5);
				xcol3nmlz=vec4(xcol3,nml.z*0.5+0.5);
				vec4 vpos=projection*view*vec4(position.xyz,1.0);
				depth=vpos.zw;
				gl_Position=vpos;
			}
