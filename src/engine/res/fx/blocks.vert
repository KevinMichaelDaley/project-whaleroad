			layout(location = 0) in highp uint L1;
			layout(location = 1) in highp uint L2;
			layout(location = 2) in vec3 pos;
			layout(location = 3) in vec2 uv;
			layout(location = 4) in uint face_index;
			uniform int x0,y0;
/*
  uint32_t L1u = uint32_t(z2%65536) +
                (uint32_t(xydiff) << 16uL) + (uint32_t(which%256) << 24uL);
  uint32_t L2u = uint32_t(which/256) + uint32_t(L1+L2<<4uL+L3<<8uL+L4<<12uL+L5<<16uL+L6<<20uL)<<8uL;
  */
			out vec3 col;
			out vec2 coord;
			out float z;
			out float zw;
			uniform mat4x4 projection, view;
			uniform vec3 sun_color;
			
            void main(){
                
				int L1x=(int(L2>>8u)>>(4*int(face_index)))%16;
				uint xydiff=(uint(L1)>>16u)&0xffu;
				
				ivec3 positioni=ivec3(int(xydiff)/16+x0,int(xydiff)%16+y0,int(L1%16384u)); 
								
				vec3 position=vec3(float(positioni.x),float(positioni.y),float(positioni.z))+pos;
				zw=pos.z/128.0;
				uint which=uint(L1>>24u)&0xffu+(uint(L2&0xffu)<<8u);
				coord=uv+vec2(float(which%256u)/256.0,0.0);
				vec4 vpos=projection*view*vec4(position.xyz,1.0);
				z=vpos.z/vpos.w;
				float Lr=float(L1x)/15.0;
				col=Lr*sun_color;
				gl_Position=vpos;
			}    
