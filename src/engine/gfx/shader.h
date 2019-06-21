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
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Magnum.h>
#include <sstream>
#include <unordered_map>
#define SHADOW_CASCADES 4
#define SMAP_RES(l) 2048
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
    shader fwd, caster, blur;
    scene* scene_;
    player* player_;
    GL::Texture2D* atlas_;
    GL::Texture2D shadowMap[SHADOW_CASCADES];
    GL::Framebuffer* shadowFramebuffer[SHADOW_CASCADES];
    
    GL::Renderbuffer depthStencil[SHADOW_CASCADES];
    
    GL::Mesh fsquad;
    
    GL::Buffer _quadBuffer;
    
    struct Vertex {
        Vector2 position;
        Vector2 textureCoordinates;
    };
public:
    block_default_forward_pass(GL::Texture2D& atlas):
        fwd{"blocks"}, caster{"caster"}, blur{"blur"}{
            
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
            atlas_=&atlas;
            fwd.texture("atlas", *atlas_);
            fwd.uniform("sun_color", Vector3(1,1,1));
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
            
   
    }
    block_default_forward_pass& set_scene(scene* s){
        scene_=s;
        fwd.texture("atlas", *atlas_);
        fwd.uniform("sun_color", Vector3(1,1,1));
        fwd.uniform("fog_color",Vector3{0.79,0.69,0.7});
        return *this;
    }
    block_default_forward_pass& set_player(player* p){
        player_=p;
        return *this;
    }
    Vector3 get_sky_color_night(float theta,int day_index){
        return {0.1,0.0,0.06};
    }
    Vector3 get_sky_color_day(float theta,int day_index){
        return {0.4,0.5,0.9};
    }
    Vector3 get_sun_color(float theta,int day_index){
        return {1.0,1.0,1};
    }
    Vector3 get_moon_color(float theta,int day_index){
         return {0.1,0.1,0.1};
    }
    float get_day_length(float year_offset){
        return 0.8;
    }
    float get_sky_angle(Vector3& sky_color, Vector3& sun_color, bool& is_day){
        float SECONDS_IN_DAY_NIGHT=3000.0;
        float DAYS_IN_YEAR=10;
        float t=timer::now();
        float time_since_sunrise=std::fmod(t,SECONDS_IN_DAY_NIGHT);
        int day_index=std::floor(t/SECONDS_IN_DAY_NIGHT);
        float SECONDS_IN_DAY=get_day_length(day_index/(float)DAYS_IN_YEAR)*SECONDS_IN_DAY_NIGHT;
        float SECONDS_IN_NIGHT=SECONDS_IN_DAY_NIGHT-SECONDS_IN_DAY;
        is_day=time_since_sunrise<SECONDS_IN_DAY;
        
        float theta=(time_since_sunrise)/SECONDS_IN_DAY;
        if(is_day){
            sky_color=get_sky_color_day(theta,day_index);
            sun_color=get_sun_color(theta,day_index);
            return theta*M_PI*0.99+M_PI*0.01;
        }
        else{
            float theta=std::fmod(time_since_sunrise,SECONDS_IN_DAY)/SECONDS_IN_NIGHT;
            sky_color=get_sky_color_night(theta,day_index);
            sun_color=get_moon_color(theta,day_index);
            return theta*M_PI*0.99+M_PI*0.01;
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
        cam2.look_at({100*(x/100)+SHADOW_DIST/2.0*cos(t),100*(y/100),SHADOW_DIST/2.0*sin(t)},{-cos(t),0.0,-sin(t)},{sin(t),0.0,cos(t)});
        return cam2;
    }
    block_default_forward_pass& draw_world_view(world_view* wv){
        auto all_chunks=wv->get_all_visible();
        
            //GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
            
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
            GL::Renderer::setClearColor({1,1,1,1});
        for(int l=0; l<SHADOW_CASCADES; ++l){
            
           // GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
            caster.uniform("view", get_sun_cam(l,l+1).view);
            caster.uniform("projection", get_sun_cam(l,l+1).projection);
            
            shadowFramebuffer[l]->clear(GL::FramebufferClear::Color|
                        GL::FramebufferClear::Depth|
                        GL::FramebufferClear::Stencil).bind();
                
            for(int i=0,e=all_chunks.size(); i<e; ++i){
                    chunk_mesh* chunk=all_chunks[i];
                    if(chunk==nullptr) continue;
                    if(l==0)
                    chunk->copy_to_gpu();
                    caster.uniform("x0", chunk->x0);
                    caster.uniform("y0", chunk->y0);

                
                    chunk->draw(&caster);
            }
                        
            
        }
        
            //GL::Renderer::setFaceCullingMode(GL::Renderer::PolygonFacing::Back);
            //GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
            //GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
            
        
        bool is_day;
        Vector3 sky_color, sun_color;
        float t=get_sky_angle(sky_color,  sun_color, is_day);
        GL::Renderer::setClearColor(sky_color);
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
            GL::defaultFramebuffer.bind();
        fwd.uniform("view", player_->get_cam().view);
        fwd.uniform("projection", player_->get_cam().projection);
        
        
        fwd.texture("shadowmap0", shadowMap[0]);
        fwd.texture("shadowmap1", shadowMap[1]);
        fwd.texture("shadowmap2", shadowMap[2]);
        fwd.texture("shadowmap3", shadowMap[3]);
        fwd.texture("atlas", *atlas_);
        fwd.uniform("light_view", get_sun_cam(0,SHADOW_CASCADES).view);
        fwd.uniform("light_projection", get_sun_cam(0,SHADOW_CASCADES).projection);
        
        
        fwd.uniform("sun_color", sun_color);
        fwd.uniform("sky_color", sky_color);
        fwd.uniform("sun_dir", Vector3{-cos(t),0,-sin(t)});
        
        fwd.uniform("fog_color",Vector3{0.7,0.69,0.79});
        for(int i=0,e=all_chunks.size(); i<e; ++i){
            chunk_mesh* chunk=all_chunks[i];
            if(chunk==nullptr) continue;
            fwd.uniform("x0", chunk->x0);
            fwd.uniform("y0", chunk->y0);
            chunk->draw(&fwd);
        }
        return *this;
    }
};

