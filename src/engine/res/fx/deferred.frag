
uniform sampler2D gbuffer_1, gbuffer_2, gbuffer_3, gbuffer_4;
uniform vec4 point[32];
uniform vec4 point_color[32];
uniform int num_lights;
uniform vec3 target_pos;
uniform float occ_radius;
uniform mat4x4 projMatrixInv, viewMatrixInv;
uniform vec3 sun_dir;
uniform float draw_dist;
uniform float sky_light_intensity;
uniform vec3 sky[9];
uniform vec3 sun[9];
uniform vec2 screen_size;
uniform sampler2D prev;
uniform float px,py;
uniform int N;
uniform sampler2D Ltex, Rtex;
uniform sampler2D SM;
uniform mat4x4 light_matrix;
uniform float ProjectionA, ProjectionB;
out vec4 frag_color;
in vec2 uv;
in vec4 vpos;


vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = viewMatrixInv * viewSpacePosition;

    return worldSpacePosition.xyz;
}

void main(){
			float z=texture2D(gbuffer_1,uv).z;
			vec3 position=WorldPosFromDepth(z).rgb;
			
			vec4 xcol1nmlx=texture2D(gbuffer_2,uv);
			vec4 xcol2nmly=texture2D(gbuffer_3,uv);
			vec4 xcol3nmlz=texture2D(gbuffer_4,uv);
			vec3 xcol=2.0*vec3(xcol1nmlx.rgb)-1.0;
			vec3 xcol2=2.0*vec3(xcol2nmly.rgb)-1.0;
			vec3 xcol3=2.0*vec3(xcol3nmlz.rgb)-1.0;
			vec3 nml=vec3(xcol1nmlx.a*2.0-1.0, xcol2nmly.a*2.0-1.0, xcol3nmlz.a*2.0-1.0);
			
			
			int face_index=int(abs(nml.x)+abs(nml.y)+abs(nml.z))*2+int(dot(nml,vec3(1,1,1))>0);
			vec3 up=vec3(0,0,1); vec3 east=vec3(1,0,0); vec3 north=vec3(0,1,0);
			vec3 t[6], b[6];
			t[0]=up;
			t[1]=up;
			t[2]=up;
			t[3]=up;
			t[4]=north;
			t[5]=north;
			b[0]=north;
			b[1]=north;
			b[2]=east;
			b[3]=east;
			b[4]=east;
			b[5]=east;
			
			mat3x3 tbn=mat3x3(t[face_index],b[face_index],nml);
			
			vec3 hemi[32];
			hemi[ 0 ]=vec3( -0.4787141370972773 , 0.8448656305238581 , -0.23881968365847245 );

			hemi[ 1 ]=vec3( 0.04774654808554598 , -0.6143266182636014 , 0.7876059123944678 );

			hemi[ 2 ]=vec3( 0.24429225468410945 , 0.7619845644821674 , 0.599750629672263 );

			hemi[ 3 ]=vec3( -0.08095325383402559 , 0.2543999637838906 , -0.9637049492040802 );

			hemi[ 4 ]=vec3( -0.3327618001662119 , -0.8844372737824384 , -0.32717012867074224 );

			hemi[ 5 ]=vec3( 0.042108340173168626 , 0.7482506721014472 , 0.66207840879128 );

			hemi[ 6 ]=vec3( 0.004435723605399039 , 0.7211327231450774 , 0.6927827364805372 );

			hemi[ 7 ]=vec3( -0.006640895947239116 , -0.3950130430665474 , 0.918651508630081 );

			hemi[ 8 ]=vec3( -0.15997975859903674 , 0.3866587667700432 , -0.908240868337504 );

			hemi[ 9 ]=vec3( -0.9028209834689003 , 0.0043938321184922545 , -0.42999414652709195 );

			hemi[ 10 ]=vec3( -0.08757521058019732 , -0.2938744340674557 , -0.9518237229095348 );

			hemi[ 11 ]=vec3( -0.27626277476489347 , 0.9332896484193454 , -0.22945437767124052 );

			hemi[ 12 ]=vec3( 0.4548965298249671 , -0.6553090439650421 , -0.6030250442981829 );

			hemi[ 13 ]=vec3( -0.9849875137915703 , -0.141855637512806 , 0.09836958768103267 );

			hemi[ 14 ]=vec3( 0.5975049579470616 , -0.7014350462714737 , 0.38855720439957847 );

			hemi[ 15 ]=vec3( 0.7758930523249639 , -0.5534343245780402 , -0.3028207716334855 );

			hemi[ 16 ]=vec3( -0.25788053759290697 , -0.08768897170117847 , -0.9621893122316332 );

			hemi[ 17 ]=vec3( 0.1351741498283913 , 0.016103591003651423 , 0.9906909828876806 );

			hemi[ 18 ]=vec3( -0.2718416689664782 , -0.9616333783335298 , 0.03692360611265196 );

			hemi[ 19 ]=vec3( -0.38493114663467554 , -0.28096298314553053 , 0.8791403838139152 );

			hemi[ 20 ]=vec3( 0.9656260296520875 , 0.25921477142183547 , -0.019340970375641083 );

			hemi[ 21 ]=vec3( 0.3686756041246029 , -0.12418714689613174 , 0.9212251904226011 );

			hemi[ 22 ]=vec3( 0.06638601939217412 , -0.7115217152256818 , 0.6995210827355854 );

			hemi[ 23 ]=vec3( -0.04190305305246149 , -0.7124370007728492 , 0.7004838713879641 );

			hemi[ 24 ]=vec3( 0.004577482685983945 , 0.005742398446922623 , 0.9999730353926232 );

			hemi[ 25 ]=vec3( -0.37582732797444773 , 0.03603452981838115 , 0.9259888402180423 );

			hemi[ 26 ]=vec3( 0.8637007139404606 , 0.0919775012656121 , 0.4955413363178425 );

			hemi[ 27 ]=vec3( -0.01859905145468669 , -0.09157284509348744 , 0.9956246729197052 );

			hemi[ 28 ]=vec3( 0.26701556036572094 , -0.9635395206175468 , 0.017154670812589385 );

			hemi[ 29 ]=vec3( 0.16327544353485793 , -0.03550669644421948 , 0.985941379619556 );

			hemi[ 30 ]=vec3( 0.4972405651045999 , -0.3903611722574599 , 0.7748354506656493 );

			hemi[ 31 ]=vec3( 0.7276644478916613 , 0.6712868413076324 , -0.14099087900195986 );
			 
			 vec3 c=(position-vec3(px,py,0))/255.0;
			 vec4 occ, sun_r, sun_g, sun_b;
			 vec3 light, skylight;
			 occ=vec4(xcol.rgb, xcol2.r);
			 light.r=2.0*(dot(sun[0], xcol)+dot(sun[1], xcol2)+dot(sun[2],xcol3));
			 light.g=2.0*(dot(sun[3], xcol)+dot(sun[4], xcol2)+dot(sun[5],xcol3));
			 light.b=2.0*(dot(sun[6], xcol)+dot(sun[7], xcol2)+dot(sun[8],xcol3));
			 skylight.r=(dot(sky[0], xcol)+dot(sky[1], xcol2)+dot(sky[2],xcol3));
			 skylight.g=(dot(sky[3], xcol)+dot(sky[4], xcol2)+dot(sky[5],xcol3));
			 skylight.b=(dot(sky[6], xcol)+dot(sky[7], xcol2)+dot(sky[8],xcol3));
			 sun_r=vec4(sun[0].rgb, sun[1].r);
			 sun_g=vec4(sun[3].rgb,  sun[4].r);
			 sun_b=vec4(sun[6].rgb,  sun[7].r);
			 sun_r+=vec4(sky_light_intensity*sky[0].rgb, sky_light_intensity*sky[1].r);
			 sun_g+=vec4(sky_light_intensity*sky[3].rgb, sky_light_intensity*sky[4].r);
			 sun_b+=vec4(sky_light_intensity*sky[6].rgb, sky_light_intensity*sky[7].r);
			 vec3 A=vec3(0.0,0.0,0.0),L=vec3(0,0,0);
                //vec4 lpos= light_matrix*vec4(position,1.0);
                //lpos/=lpos.w;
                //light*=float(texture(SM, 0.5*lpos.rg+0.5).r*2.0-1.0>=lpos.z);
		
			 for(int i=0; i<32; i+=1){
					vec3 dir=tbn*hemi[i];
					vec4 proj=vec4(0.282095f,-0.488603f,0.488603f,-0.488603f)*vec4(1.0,dir.yzx);
					vec3 samp=vec3(0.5*dir.x,0.5*dir.y,dir.z);
					
					 
					float dist=dot(proj,occ);
					 
					 
					
					
					 
				
					 
					vec3 coffs=c.xyz+dist*samp;
					
				
					vec2 cxy=vec2(((coffs.x)/N+coffs.y/(N*N))+0.5,0.5*coffs.z+0.5);
					vec4 prop=2.0*texture(Ltex, cxy)-1.0;
					vec4 refl=texture(Rtex, cxy);
					
					float df=max(dot(dir,nml),0.9);	
					
					float v=df*sqrt(1.0-min(dir.x*dir.x,min(dir.y*dir.y,dir.z*dir.z)));
					v*=1.0/(dist*dist*0.5+1.0);
					 for(int k=0; k<min(5,num_lights); k+=1){
						
						 vec3 cL=((position)-point[k].rgb)/255.0;
						 
						 float atten1=point_color[k].a*20.0;
						 float atten_scale=atten1/255.0;
						 vec3 sh=-cL+samp*dist;
						 
						 float falloff=1.0;
						 vec3 shn=normalize(sh);
						 
						 
						 vec4 proj3=vec4(0.282095f,-0.488603f,0.488603f,-0.488603f)*vec4(1.0,shn.yzx);
						 
						 float shad2=max(float(dot(proj3,prop)>=0.9*(length(sh))),0.0);
						 
						 float df2=shad2*df;
						 
						 A+=0.4*exp(-dist*dist*255.0*atten1)*point_color[k].rgb*df2*falloff*max(refl.xyz*v,0.01);
					}
					 
					A.r=A.r+0.4*dot(sun_r,prop)*df*max(prop.b,0.1)*max(refl.w*refl.x*v,0.01);
					A.g=A.g+0.4*dot(sun_g,prop)*df*max(prop.b,0.1)*max(refl.w*refl.y*v,0.01);
					A.b=A.b+0.4*dot(sun_b,prop)*df*max(prop.b,0.1)*max(refl.w*refl.z*v,0.01);
             }
			  
			  L=16*(light*max(dot(sun_dir,nml),0.5*occ.b) + skylight)*(1+max(occ.b,0))+max(A*0.8, 0.0);
			
					for(int k=0; k<min(10,num_lights); k+=1){
					 
						vec3 sh=(point[k].rgb-(position));
						float atten1=point_color[k].a*20.0;
						 float atten_scale=atten1/255.0;
						 float falloff=1.0;
						vec3 shn=normalize(sh);
						
						
						vec4 proj2=vec4(0.282095f,-0.488603f,0.488603f,-0.488603f)*vec4(1.0,shn.yzx);
						float shad=max(float(dot(proj2,occ)>=0.9*length((sh)))/255,0.1);
						float df2=shad*min( (falloff), 1.0)*max(dot(shn,nml),0.9);
						L+=4/exp(dot(sh,sh)/5.0*atten_scale+1.0)*point_color[k].rgb*df2;
					}
					
			 frag_color.rgb=L;
			 frag_color.a=1.0;
    }
