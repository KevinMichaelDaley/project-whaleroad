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
#include <Magnum/Math/Color.h>
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

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    std::string src_vert = read_text_file(shader_name + ".vert");
    std::string src_frag = read_text_file(shader_name + ".frag");
    vert.addSource(src_vert);
    frag.addSource(src_frag);

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
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
    shader fwd;
    scene* scene_;
    player* player_;
    GL::Texture2D* atlas_;
public:
    block_default_forward_pass(GL::Texture2D& atlas):
        fwd{"../../src/engine/res/fx/blocks"}{
            atlas_=&atlas;
            fwd.texture("atlas", *atlas_);
            fwd.uniform("sun_color", Vector3(1,1,1));
        }
    block_default_forward_pass& set_scene(scene* s){
        scene_=s;
        fwd.texture("atlas", *atlas_);
        fwd.uniform("sun_color", Vector3(1,1,1));
        return *this;
    }
    block_default_forward_pass& set_player(player* p){
        player_=p;
        return *this;
    }
    block_default_forward_pass& draw_world_view(world_view* wv){
        auto all_chunks=wv->get_all_visible();
        fwd.uniform("view", player_->get_cam().view);
        fwd.uniform("projection", player_->get_cam().projection);
        for(int i=0,e=all_chunks.size(); i<e; ++i){
            chunk_mesh* chunk=all_chunks[i];
            chunk->copy_to_gpu();
            fwd.uniform("x0", chunk->x0);
            fwd.uniform("y0", chunk->y0);
            chunk->draw(&fwd);
        }
        return *this;
    }
};

