			layout(location = 0) in uint L1;
			layout(location = 1) in vec3 pos;
			layout(location = 2) in vec2 uv;
			layout(location = 3) in vec3 nml;
			uniform int x0,y0;

			out vec3 col;
			out vec2 coord;
			out float z;
			uniform mat4x4 projection, view;
			uniform vec3 sun_color;
			
            void main(){
				
				
				uint L1x=L1;
				uint xydiff=(L1>>14u)&0xffu;
				
				ivec3 positioni=ivec3(int(xydiff)/16+x0,int(xydiff)%16+y0,int(L1>>14u)&0xff); 
								
				vec3 position=vec3(float(positioni.x),float(positioni.y),float(positioni.z))+pos;
				int which=int(L1>>22u);
				coord=uv+vec2(float(which)/256.0,0.0);
				vec4 vpos=projection*view*vec4(position.xyz,1.0);
				z=vpos.z/vpos.w;
				col=float(L1x)*sun_color/255.0;
				gl_Position=vpos;
				
			}
