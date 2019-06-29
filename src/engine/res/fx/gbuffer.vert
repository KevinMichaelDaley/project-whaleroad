            layout(location = 0) in highp uint L1;
			layout(location = 1) in highp uint L2;
			layout(location = 2) in vec3 pos;
			layout(location = 3) in vec2 uv;
			layout(location = 4) in uint face_index;
			layout(location = 5) in uint face_neighbor1;
			layout(location = 6) in uint face_neighbor2;
			uniform int x0,y0,z0;
/*  uint32_t L1u = uint32_t(z2%65536) +
                (uint32_t(xydiff) << 16uL) + (uint32_t(which%256) << 24uL);
  uint32_t L2u = uint32_t(which/256) + uint32_t(L1+L2<<4uL+L3<<8uL+L4<<12uL+L5<<16uL+L6<<20uL)<<8uL;
  */
			out vec4 gbuffer;
			out float w;
			uniform mat4x4 projection, view;
			
            void main(){
                
				uint which=uint(L1>>13u)%8192u;
				int L1x=int((L2>>(6u*(face_index)))%64u)*int(face_index<5u);
				L1x+=int((L1>>21u)&0xffu)*int(face_index==5u);
				
				int L1y=int((L2>>(6u*(face_neighbor1)))%64u)*int(face_neighbor1<5u);
				L1y+=int((L1>>21u)&0xffu)*int(face_neighbor1==5u);
				
				int L1z=int((L2>>(6u*(face_neighbor2)))%64u)*int(face_neighbor2<5u);
				L1z+=int((L1>>21u)&0xffu)*int(face_neighbor2==5u);
				/*int L1y=(int(L2>>8u)>>(4*int(face_neighbor1)))%16;
				int L1z=(int(L2>>8u)>>(4*int(face_neighbor2)))%16;*/
				uint xydiff=(uint(L1)>>5u)&0xffu;
				
				ivec3 positioni=ivec3(int(xydiff)/16+x0,int(xydiff)%16+y0,int(L1%32u)+z0); 
								
				vec3 position=vec3(float(positioni.x),float(positioni.y),float(positioni.z))+pos;
				
				vec4 vpos=projection*view*vec4(position.xyz,1.0);

				float Lr=max(L1y,max(L1x,L1z))/64.0;
                vec2 coord=uv+vec2(float(which%256u)/256.0,0.0);
                gbuffer.x=coord.x;
                gbuffer.y=coord.y;
                gbuffer.z=vpos.z;
                gbuffer.w=Lr;
                w=vpos.w;   
				gl_Position=vpos;
			}    
