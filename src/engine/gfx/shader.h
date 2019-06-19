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
    shader fwd, caster;
    scene* scene_;
    player* player_;
    GL::Texture2D* atlas_;
    GL::Texture2D shadowMap[8];
    GL::Framebuffer* shadowFramebuffer[8];
    
    GL::Renderbuffer depthStencil[8];
    
public:
    block_default_forward_pass(GL::Texture2D& atlas):
        fwd{"blocks"}, caster{"caster"}{
            atlas_=&atlas;
            fwd.texture("atlas", *atlas_);
            fwd.uniform("sun_color", Vector3(1,1,1));
            for(int l=0; l<8; ++l){
                depthStencil[l].setStorage(GL::RenderbufferFormat::DepthStencil,{8192,8192});
            (shadowMap[l] = GL::Texture2D{})
            .setStorage(1, GL::TextureFormat::R32F, {8192,8192})
            .setMinificationFilter(GL::SamplerFilter::Linear)
            .setMagnificationFilter(GL::SamplerFilter::Linear);
                    (shadowFramebuffer[l]=new GL::Framebuffer{Range2Di{{}, {8192,8192}}})->attachTexture(GL::Framebuffer::ColorAttachment(0), shadowMap[l], 0)
                    
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
        fwd.uniform("fog_color",Vector3{0.8,0.69,0.7});
        return *this;
    }
    block_default_forward_pass& set_player(player* p){
        player_=p;
        return *this;
    }
    camera get_sun_cam(int i, int j){
        camera cam2;
        cam2.set_ortho({3000.0,3000.0}, 0.1+(constants::WORLD_HEIGHT+5)*i/8.0, 0.1+(constants::WORLD_HEIGHT+5)*(j)/8.0);
        cam2.look_at(Vector3{0,0,constants::WORLD_HEIGHT+1},{sin(0.01),0,-cos(0.01)},{cos(0.01),0,sin(0.01)});
        return cam2;
    }
    block_default_forward_pass& draw_world_view(world_view* wv){
        auto all_chunks=wv->get_all_visible();
        
        

        for(int l=0; l<8; ++l){
            
            caster.uniform("view", get_sun_cam(l,l+1).view);
            caster.uniform("projection", get_sun_cam(l,l+1).projection);
            shadowFramebuffer[l]->clear(GL::FramebufferClear::Color|
                        GL::FramebufferClear::Depth|
                        GL::FramebufferClear::Stencil).bind();
                
            for(int i=0,e=all_chunks.size(); i<e; ++i){
                    chunk_mesh* chunk=all_chunks[i];
                    chunk->copy_to_gpu();
                    caster.uniform("x0", chunk->x0);
                    caster.uniform("y0", chunk->y0);

                
                    chunk->draw(&caster);
            }
        }
        
        GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
            GL::defaultFramebuffer.bind();
        fwd.uniform("view", player_->get_cam().view);
        fwd.uniform("projection", player_->get_cam().projection);
        fwd.texture("shadowmap0", shadowMap[0]);
        fwd.texture("shadowmap1", shadowMap[1]);
        fwd.texture("shadowmap2", shadowMap[2]);
        fwd.texture("shadowmap3", shadowMap[3]);
        fwd.texture("shadowmap4", shadowMap[4]);
        fwd.texture("shadowmap5", shadowMap[5]);
        fwd.texture("shadowmap6", shadowMap[6]);
        fwd.texture("shadowmap7", shadowMap[7]);
        fwd.texture("atlas", *atlas_);
        fwd.uniform("light_view", get_sun_cam(0,8).view);
        fwd.uniform("light_projection", get_sun_cam(0,8).projection);
        
        
        
        
        fwd.uniform("sun_color", Vector3(1,1,1));
        fwd.uniform("fog_color",Vector3{0.8,0.69,0.7});
        for(int i=0,e=all_chunks.size(); i<e; ++i){
            chunk_mesh* chunk=all_chunks[i];
            fwd.uniform("x0", chunk->x0);
            fwd.uniform("y0", chunk->y0);
            chunk->draw(&fwd);
        }
        return *this;
    }
};

