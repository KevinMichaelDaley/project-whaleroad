#pragma once
#include "chunk_mesh.h"
#include "vox/world.h"
#include "common/scene.h"
#include "phys/player.h"
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>

#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/SampleQuery.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Magnum.h>
#include <sstream>
#include <unordered_map>
#define SHADOW_CASCADES 4
#define SMAP_RES(l) 1920
#define SHADOW_DIST 400.0
std::string read_text_file(std::string filename) {
  FILE *f = fopen(filename.c_str(), "r");
  fseek(f, 0, SEEK_END);
  std::string a;
  a.resize(ftell(f), 0);
  fseek(f, 0, SEEK_SET);
  fread(&(a[0]), 1, a.size(), f);
  return a;
}

using namespace Magnum;

class shader : public GL::AbstractShaderProgram {
public:
  shader(std::string shader_name) {
#ifndef __ANDROID__
    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};
#else
    
    GL::Shader vert{GL::Version::GLES300, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GLES300, GL::Shader::Type::Fragment};
#endif
    
    const Utility::Resource rs{"art"};
    vert.addSource(rs.get(shader_name + ".vert"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
    
    frag.addSource(rs.get(shader_name + ".frag"));
    CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());

    attachShader(vert);
    attachShader(frag);

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());
  }

private:
  std::unordered_map<std::string, Int> uniforms;
  std::unordered_map<std::string, int> samplers_;

public:
  template <typename T> shader &uniform(std::string s, T v) {
    if (!uniforms.count(s)) {
      uniforms[s] = uniformLocation(s);
    }
    setUniform(uniforms[s], v);
    return *this;
  }

  int texture(std::string s, GL::Texture2D &v) {
    int sampler;
    if (samplers_.count(s)) {
      sampler = samplers_[s];
    } else {
      sampler = (samplers_.size());
      samplers_[s] = sampler;
    }
    v.bind(sampler);
    uniform(s, sampler);
    return sampler;
  }
};


class block_default_forward_pass{
    
    typedef GL::Attribute<0, Vector2> pos;
    typedef GL::Attribute<1, Vector2> uv;
    typedef GL::Attribute<0, Vector3> cpos;
    shader deferred, passthrough, gbuffer,sky;
    scene* scene_;
    player* player_;
    GL::Texture2D* atlas_;
    GL::Texture2D gbuffertex;
    GL::Texture2D shadowMap[SHADOW_CASCADES];
    GL::Framebuffer* shadowFramebuffer[SHADOW_CASCADES];
    GL::Framebuffer GBuffer;
    
    GL::Renderbuffer depthStencil[SHADOW_CASCADES];
    
    GL::Renderbuffer depthStencil1;
    int frame;
    GL::Mesh fsquad, fullchunk;
    
    GL::Buffer _quadBuffer, _cubeBuffer;
    std::vector<int> vis;
    std::vector<int> indices;
    std::vector<double> min_depth;
    std::vector<GL::SampleQuery*> q;
    struct Vertex {
        Vector2 position;
        Vector2 textureCoordinates;
    };
public:
    block_default_forward_pass(GL::Texture2D& atlas):
        GBuffer(GL::defaultFramebuffer.viewport()),
        deferred{"deferred"}, passthrough{"passthrough"}, gbuffer{"gbuffer"}, sky{"sky"}{
            frame=0;
            const Vertex lb{ { -1, -1 },{ 0, 0 } };         // left bottom
            const Vertex lt{ { -1,  1 },{ 0, 1 } };         // left top
            const Vertex rb{ { 1,  -1 },{ 1, 0 } };         // right bottom
            const Vertex rt{ { 1,  1}, { 1, 1 } };         // right top

            const Vertex quadData[]{
                lb, rt, lt,
                rt, lb, rb
            };
        
            _quadBuffer.setData(quadData, GL::BufferUsage::StaticDraw);
            
            fsquad
                .setPrimitive(GL::MeshPrimitive::Triangles)
                .setCount(6)
                .addVertexBuffer(_quadBuffer, 0,
                    pos{},
                    uv{});
                
            float cube[]={
                -1.0f,-1.0f,-1.0f, // triangle 1 : begin
                -1.0f,-1.0f, 1.0f,
                -1.0f, 1.0f, 1.0f, // triangle 1 : end
                1.0f, 1.0f,-1.0f, // triangle 2 : begin
                -1.0f,-1.0f,-1.0f,
                -1.0f, 1.0f,-1.0f, // triangle 2 : end
                1.0f,-1.0f, 1.0f,
                -1.0f,-1.0f,-1.0f,
                1.0f,-1.0f,-1.0f,
                1.0f, 1.0f,-1.0f,
                1.0f,-1.0f,-1.0f,
                -1.0f,-1.0f,-1.0f,
                -1.0f,-1.0f,-1.0f,
                -1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f,-1.0f,
                1.0f,-1.0f, 1.0f,
                -1.0f,-1.0f, 1.0f,
                -1.0f,-1.0f,-1.0f,
                -1.0f, 1.0f, 1.0f,
                -1.0f,-1.0f, 1.0f,
                1.0f,-1.0f, 1.0f,
                1.0f, 1.0f, 1.0f,
                1.0f,-1.0f,-1.0f,
                1.0f, 1.0f,-1.0f,
                1.0f,-1.0f,-1.0f,
                1.0f, 1.0f, 1.0f,
                1.0f,-1.0f, 1.0f,
                1.0f, 1.0f, 1.0f,
                1.0f, 1.0f,-1.0f,
                -1.0f, 1.0f,-1.0f,
                1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f,-1.0f,
                -1.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 1.0f,
                -1.0f, 1.0f, 1.0f,
                1.0f,-1.0f, 1.0f
            };
            for(int i=0; i<12; ++i){
                cube[i*3+0]=(0.5*cube[i*3+0]+0.5)*constants::CHUNK_WIDTH;
                cube[i*3+1]=(0.5*cube[i*3+1]+0.5)*constants::CHUNK_WIDTH;
                cube[i*3+2]=(0.5*cube[i*3+2]+0.5)*constants::CHUNK_HEIGHT;
            }
            _cubeBuffer.setData(cube, GL::BufferUsage::StaticDraw);
            
            fullchunk
                .setPrimitive(GL::MeshPrimitive::Triangles)
                .setCount(12)
                .addVertexBuffer(_cubeBuffer, 0,
                    cpos{});
            
            atlas_=&atlas;
            /*
            for(int l=0; l<SHADOW_CASCADES; ++l){
               
            
                depthStencil[l].setStorage(GL::RenderbufferFormat::DepthStencil,{SMAP_RES(l),SMAP_RES(l)});
            (shadowMap[l] = GL::Texture2D{})
            .setStorage(1, GL::TextureFormat::R32F, {SMAP_RES(l),SMAP_RES(l)})
            .setMinificationFilter(GL::SamplerFilter::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear);
                    (shadowFramebuffer[l]=new GL::Framebuffer{Range2Di{{}, {SMAP_RES(l),SMAP_RES(l)}}})->attachTexture(GL::Framebuffer::ColorAttachment(0), shadowMap[l], 0)
                    
                .attachRenderbuffer(
        GL::Framebuffer::BufferAttachment::DepthStencil, depthStencil[l])
                .mapForDraw({{ 0, {GL::Framebuffer::ColorAttachment( 0 )} }});
                 auto status =  shadowFramebuffer[l]->checkStatus( Magnum::GL::FramebufferTarget::Draw );
                    if( status != Magnum::GL::Framebuffer::Status::Complete )
                    {
                        Corrade::Utility::Error() << status;
                        std::exit( 0 );
                    }
            }
            */
             depthStencil1.setStorage(GL::RenderbufferFormat::DepthStencil, {GL::defaultFramebuffer.viewport().sizeX(),GL::defaultFramebuffer.viewport().sizeY()});
            (gbuffertex = GL::Texture2D{})
            .setStorage(1, GL::TextureFormat::RGBA16F, {GL::defaultFramebuffer.viewport().sizeX(),GL::defaultFramebuffer.viewport().sizeY()})
            .setMinificationFilter(GL::SamplerFilter::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear);
                GBuffer.attachTexture(GL::Framebuffer::ColorAttachment(0), gbuffertex, 0)
                .attachRenderbuffer(GL::Framebuffer::BufferAttachment::DepthStencil, depthStencil1)
                .mapForDraw({{ 0, {GL::Framebuffer::ColorAttachment( 0 )} }});
                auto status =  GBuffer.checkStatus( Magnum::GL::FramebufferTarget::Draw );
                   if( status != Magnum::GL::Framebuffer::Status::Complete )
                    {
                        Corrade::Utility::Error() << status;
                        std::exit( 0 );
                    }
            
            
   
    }
    block_default_forward_pass& set_scene(scene* s){
        scene_=s;
        deferred.texture("atlas", *atlas_);
        deferred.uniform("sun_color", Vector3(1,1,1));
        deferred.uniform("fog_color",Vector3{0.79,0.69,0.7});
        return *this;
    }
    block_default_forward_pass& set_player(player* p){
        player_=p;
        return *this;
    }
    Vector3 get_sky_color_night(float theta,int day_index){
        Vector3 night_color= {0.1,0.0,0.06};
        
        Vector3 dawn_color=Vector3{0.4,0.5,0.6};
        Vector3 dusk_color=Vector3{0.4,0.4,0.45};
        
        if(theta<0.5){
            return Math::lerp(dusk_color,night_color,theta*theta*4.0);
        }
        else{
            return Math::lerp(night_color,dawn_color, theta*theta*4.0-1.0);
        }
    }
    Vector3 get_sky_color_day(float theta,int day_index){
        Vector3 dawn_color=Vector3{0.4,0.5,0.6};
        Vector3 day_color=Vector3{0.4,0.5,0.9};
        Vector3 dusk_color=Vector3{0.4,0.4,0.45};
        
        if(theta<0.5){
            return Math::lerp(dawn_color,day_color,theta*theta*4.0);
        }
        else{
            return Math::lerp(day_color,dusk_color, theta*theta*4.0-1.0);
        }
    }
    Vector3 get_sun_color(float theta,int day_index){
        Vector3 dawn_color=Vector3{0.99,0.5,0.6};
        Vector3 day_color=Vector3{0.5,0.5,0.35};
        Vector3 dusk_color=Vector3{0.5,0.55,0.45};
        if(theta<0.5){
            return Math::lerp(dawn_color,day_color,theta*theta*4.0);
        }
        else{
            return Math::lerp(day_color,dusk_color, theta*theta*4.0-1.0);
        }
    }
    Vector3 get_moon_color(float theta,int day_index){
        
        Vector3 dawn_color=Vector3{0.99,0.5,0.6};
        Vector3 night_color=Vector3{0.15,0.15,0.15};
        Vector3 dusk_color=Vector3{0.5,0.55,0.45};
        
        if(theta<0.5){
            return Math::lerp(dusk_color,night_color, theta*theta*4.0);
        }
        else{
            return Math::lerp(night_color,dawn_color,  theta*theta*4.0-1.0);
        }
    }
    float get_day_length(float year_offset){
        return 0.8;
    }
    float get_sky_angle(Vector3& sky_color, Vector3& sun_color, bool& is_day){
        float SECONDS_IN_DAY_NIGHT=300.0;
        float DAYS_IN_YEAR=10;
        float t=timer::now()+SECONDS_IN_DAY_NIGHT/3.0;
        float time_since_sunrise=std::fmod(t,SECONDS_IN_DAY_NIGHT);
        int day_index=std::floor(t/SECONDS_IN_DAY_NIGHT);
        float SECONDS_IN_DAY=get_day_length(day_index/(float)DAYS_IN_YEAR)*SECONDS_IN_DAY_NIGHT;
        float SECONDS_IN_NIGHT=SECONDS_IN_DAY_NIGHT-SECONDS_IN_DAY;
        is_day=time_since_sunrise<SECONDS_IN_DAY;
        
        float theta=(time_since_sunrise)/SECONDS_IN_DAY;
        if(is_day){
            sky_color=get_sky_color_day(theta,day_index);
            sun_color=get_sun_color(theta,day_index);
            return theta*M_PI;
        }
        else{
            float theta=std::fmod(time_since_sunrise,SECONDS_IN_DAY)/SECONDS_IN_NIGHT;
            sky_color=get_sky_color_night(theta,day_index);
            sun_color=get_moon_color(theta,day_index);
            return theta*M_PI;
        }
    }
    camera get_sun_cam(int i, int j){
        camera cam2;
        int x=player_->get_position().x();
        int y=player_->get_position().y();
        cam2.set_ortho({SHADOW_DIST,SHADOW_DIST}, 0.1+(SHADOW_DIST*2.0)*i/double(SHADOW_CASCADES), 0.1+(SHADOW_DIST*2.0)*(j)/double(SHADOW_CASCADES));
        Vector3 tmp1,tmp2;
        bool tmp3;
        float t=get_sky_angle(tmp1,tmp2,tmp3);
        cam2.look_at({100*(x/100)+SHADOW_DIST*sin(t),100*(y/100),SHADOW_DIST*cos(t)},{-sin(t),0.0,-cos(t)},{cos(t),0.0,sin(t)});
        return cam2;
    }
    float cross(Vector2 x, Vector2 y){
        return x.x()*y.y()-x.y()*y.x();
    }
    void drawTri(uint16_t* cull_array, int res, Vector3 vt1, Vector3 vt2, Vector3 vt3, int i){
            /* spanning vectors of edge (v1,v2) and (v1,v3) */
            vt1*=res/2.0;
            vt2*=res/2.0;
            vt3*=res/2.0;
            int maxX = (int)std::floor(std::max(vt1.x(), std::max(vt2.x(), vt3.x())));
            int minX = (int)std::floor(std::min(vt1.x(), std::min(vt2.x(), vt3.x())));
            int maxY = (int)std::floor(std::max(vt1.y(), std::max(vt2.y(), vt3.y())));
            int minY = (int)std::floor(std::min(vt1.y(), std::min(vt2.y(), vt3.y())));
            Vector2 vs1 = Vector2{(vt2.x() - vt1.x()), (vt2.y() - vt1.y())};
            Vector2 vs2 = Vector2{(vt3.x() - vt1.x()), (vt3.y() - vt1.y())};
            if((vt1.z()<=0.0 || vt1.z()>1.0) &&
               (vt2.z()<=0.0 || vt2.z()>1.0) &&
               (vt3.z()<=0.0 || vt3.z()>1.0)){
                return;
            }
            float z1inv=1.0/vt1.z();
            float z2inv=1.0/vt2.z();
            float vcross=cross(vs1,vs2);
            for (int x = minX; x <= maxX; x+=1)
            {
                for (int y = minY; y <= maxY; y++)
                {
                    if(x<-res/2 || x>=res/2 || y<-res/2 || y>=res/2) continue;
                    Vector2 q = Vector2(x - vt1.x(), y - vt1.y());

                    float s = (float)cross(q,vs2) / vcross;
                    float t = (float)cross(vs1,q) / vcross;

                    if ( (s >= 0) && (t >= 0) && (s + t <= 1))
                    { 
                        float lambda=q.x()/vs1.x();
                        float invz=(1.0-lambda)/z1inv+lambda/z2inv;
                        int ix=std::floor(x+res/2)*res+std::floor(y+res/2);
                        uint16_t z1=std::min(uint16_t(254u),(uint16_t)(255.0f*(0.5f/invz+0.5f)));
                        uint16_t p0=cull_array[ix];
                        uint16_t p1=z1<<8+i;
                        cull_array[ix]=std::min(uint16_t(p0),uint16_t(p1));
                    }
                }
            }
            return;
    }
    void rasterize(chunk_mesh* chunk, int z0, camera cam, uint16_t* cull_array, int res, uint8_t index, bool whole_box_only=true){
        Matrix4 viewproj=cam.projection*cam.view;
        int x0=chunk->x0;
        int y0=chunk->y0;
        int N=constants::CHUNK_HEIGHT*constants::CHUNK_WIDTH*constants::CHUNK_WIDTH;
        bool full=(chunk->volume==constants::CHUNK_WIDTH*constants::CHUNK_HEIGHT*constants::CHUNK_WIDTH);
        if(whole_box_only && !full){
                return;
        }
        if(full){
            for(int face=0; face<6; ++face){
                Vector3 v[4];
                for(int vert=0; vert<4; ++vert){
                    Vector4 vi=viewproj*(Vector4{x0+FacesOffset[face][vert][0]*constants::CHUNK_WIDTH,y0+FacesOffset[face][vert][1]*constants::CHUNK_WIDTH,z0+FacesOffset[face][vert][2]*constants::CHUNK_HEIGHT,1.0});
                    v[vert]=Vector3{vi.x(),vi.y(),vi.z()}/vi.w();
                }
                drawTri(cull_array,res,v[1],v[3],v[0],index);
                drawTri(cull_array,res,v[2],v[0],v[1],index);
            }
            return;
        }
        
            
            
        for(int i=0,e=chunk->Nverts[z0]; i<e; ++i){
            uint32_t L1=chunk->verts[N*z0+i].L2;
            int xy=(L1>>16)&0xff;
            int x=(xy>>4)+x0;
            int y=(xy&0xf)+y0;
            int z=(L1&0xffff);
            for(int face=0; face<6; ++face){
                Vector3 v[4];
                for(int vert=0; vert<4; ++vert){
                    Vector4 vi=viewproj*(Vector4{x+FacesOffset[face][vert][0],y+FacesOffset[face][vert][1],z+FacesOffset[face][vert][2],1.0});
                    v[vert]=Vector3{vi.x(),vi.y(),vi.z()}/vi.w();
                }
                drawTri(cull_array,res,v[1],v[3],v[0],index);
                drawTri(cull_array,res,v[2],v[0],v[1],index);
            }
        }
        return;
    }
    block_default_forward_pass& draw_world_view(world_view* wv){
        auto all_chunks=wv->get_all_visible();
        
        camera cam=player_->get_cam();
        Matrix4 viewproj=cam.projection*cam.view;
        //camera sun_cam=get_sun_cam(0,SHADOW_CASCADES);
        gbuffer.uniform("view", cam.view);
        gbuffer.uniform("projection", cam.projection);   
        
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
        GBuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();
        
        fsquad.draw(sky);
        indices.clear();
        min_depth.clear();
        vis.resize(all_chunks.size(),0);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        for(int i=0,e=all_chunks.size()*(constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT); i<e; ++i){
            int j=i/(constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT);
            int z0=i%(constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT);
            indices.push_back(i);
            float z=all_chunks[j]->min_depth(cam,z0);
            min_depth.push_back(z);            
           
        }
         while(q.size()<all_chunks.size()*(constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT)){
                q.push_back(new GL::SampleQuery{GL::SampleQuery::Target::AnySamplesPassed});
        }
                    
        passthrough.uniform("transform", viewproj);  
        std::sort(indices.begin(), indices.end(), [this](int i, int j){return this->min_depth[i]<this->min_depth[j];});
        for(int j=0,e=indices.size(); j<e; ++j){
                int i=indices[j]/(constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT);
                int z0=indices[j]%(constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT);
                chunk_mesh* chunk=all_chunks[i];
            
                if(chunk==nullptr) continue;
            
                int ixc=indices[j];
                if(!chunk->is_visible(cam,z0)){ vis[ixc]=0; continue;}
                vis[ixc]++;
                if(vis[ixc]<2  ||  q[ixc]->result<bool>()){
                    vis[ixc]=true;
                    gbuffer.uniform("x0", chunk->x0);
                    gbuffer.uniform("y0", chunk->y0);
                    gbuffer.uniform("z0",z0*constants::CHUNK_HEIGHT);
                    chunk->copy_to_gpu(z0);
                    q[ixc]->begin();
                    chunk->draw(&gbuffer,z0);
                    q[ixc]->end();
                }   
                else{
                    glColorMask(false,false,false,false);
                    glDepthMask(false);
                    passthrough.uniform("x0",chunk->x0);
                    passthrough.uniform("y0",chunk->y0);
                    passthrough.uniform("z0",z0*constants::CHUNK_HEIGHT);
                    q[ixc]->begin();
                    fullchunk.draw(passthrough);
                    q[ixc]->end();
                    glColorMask(true,true,true,true);
                    glDepthMask(true);
                }
         }
         /*
            //GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::setClearColor({1,1,1,1});
        for(int l=0; l<SHADOW_CASCADES; ++l){
            
            camera slice_cam=get_sun_cam(0,SHADOW_CASCADES);
           // GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
            caster.uniform("view", slice_cam.view);
            caster.uniform("projection", slice_cam.projection);
            
            shadowFramebuffer[l]->clear(GL::FramebufferClear::Color|
                        GL::FramebufferClear::Depth|
                        GL::FramebufferClear::Stencil).bind();
                
            for(int i=0,e=all_chunks.size(); i<e; ++i){
                    chunk_mesh* chunk=all_chunks[i];
                    if(chunk==nullptr) continue;
                    
                    caster.uniform("x0", chunk->x0);
                    caster.uniform("y0", chunk->y0);
                    for(int z0=0; z0<constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT; ++z0){
                        if(!chunk->is_visible(slice_cam,z0)) continue;
                        int ixc=i*constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT+z0;
                        if(q[ixc]->result<bool>() || (frame++)%all_chunks.size()==i){
                            if(!chunk->is_visible(cam,z0)) q[ixc+constants::WORLD_HEIGHT*all_chunks.size()/constants::CHUNK_HEIGHT]->begin();
                            chunk->draw(&caster,z0);
                            if(!chunk->is_visible(cam,z0)) q[ixc+constants::WORLD_HEIGHT*all_chunks.size()/constants::CHUNK_HEIGHT]->end();
                        }
                        q[ixc]->endConditionalRender();
                    }
                    
            }
        }
        */
           
            GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
            //GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
            
        
        bool is_day;
        Vector3 sky_color, sun_color;
        float t=get_sky_angle(sky_color,  sun_color, is_day);
        GL::Renderer::setClearColor(sky_color);
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
            GL::defaultFramebuffer.bind();
        
        
        deferred.texture("atlas", *atlas_);
        deferred.texture("gbuffer", gbuffertex);
        
        deferred.uniform("sun_color", sun_color);
        deferred.uniform("sky_color", sky_color);
        deferred.uniform("fog_color",Vector3{0.4,0.3,0.2});
        /*
        for(int i=0,e=all_chunks.size(); i<e; ++i){
            chunk_mesh* chunk=all_chunks[i];
            if(chunk==nullptr) continue;
            deferred.uniform("x0", chunk->x0);
            deferred.uniform("y0", chunk->y0);
            for(int z0=0; z0<constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT; ++z0){
                if(!chunk->is_visible(cam,z0)) continue;
                
                int ixc=i*constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT+z0;
                q[ixc]->beginConditionalRender(GL::SampleQuery::ConditionalRenderMode::Wait);
                chunk->draw(&deferred,z0);
                q[ixc]->endConditionalRender();
            }
        }*/
        fsquad.draw(deferred);
        /*
        for(int i=0,e=all_chunks.size(); i<e; ++i){
            
            for(int z0=0; z0<constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT; ++z0){
                
                int ixc=i*constants::WORLD_HEIGHT/constants::CHUNK_HEIGHT+z0;
                std::swap(q[ixc], q[ixc+constants::WORLD_HEIGHT*all_chunks.size()/constants::CHUNK_HEIGHT]);
            }
        }*/
        return *this;
    }
    ~block_default_forward_pass(){
        for(int i=0; i<SHADOW_CASCADES; ++i){
            
            delete shadowFramebuffer[i];
        }
        
        for(int i=0,e=q.size(); i<e; ++i){
            delete q[i];
        }
    }
};

