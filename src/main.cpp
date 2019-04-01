
#include "common/timer.h"
#include "gfx/shader.h"
#include "gfx/chunk_mesh.h"
#include "phys/player.h"
#include "vox/world.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Renderbuffer.h"
#include "Magnum/PixelFormat.h"
#include <Magnum/Timeline.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include "Magnum/GL/RenderbufferFormat.h"
#include "Magnum/ImageView.h"

#include <chrono>

using namespace Magnum;


class game : public Platform::Sdl2Application {
private:
  block_default_forward_pass* block_pass;
  world_view *wv_;
  world* w_;
  scene scene_;
  std::string player_name;
  GL::Texture2D atlas;
  char** argv; int argc;
public:
  explicit game(const Arguments &arguments)
      :  Platform::Sdl2Application(
                                  arguments,
                                  Configuration{}.setSize({1920, 1080})
                                  ) ,
          atlas(Texture2D{Magnum::NoCreate}),
          argc(arguments.argc),
          argv(arguments.argv)
      {
    setSwapInterval(1);
    PluginManager::Manager<Trade::AbstractImporter> manager;

    player_name = arguments.argc > 1 ? arguments.argv[1] : "new_player";
    std::unique_ptr<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("AnyImageImporter");
    importer->openFile(std::string("atlas.tga"));
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    atlas.setWrapping(GL::SamplerWrapping::Repeat)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGB8, image->size())
        .setSubImage(0, {}, *image)
        .generateMipmap();
    block_pass=new block_default_forward_pass(atlas);
  }
  void load_world(std::string name) {
    bool new_world;
    w_ = world::load_or_create(name, new_world);
    if (new_world) {
      w_->save_all();
    }
    assert(argc > 1);
    std::string player_name = std::string(argv[1]);
  }
  void menu() {
    std::cout << "Enter a world name to start:";
    std::string wld_name;
    std::cin >> wld_name;
    load_world(wld_name);
    spawn();
  }
  void track_player() {
    player *player0 = scene_.get_player(0);
    Vector3 v = player0->get_position();
    wv_->update_center(v);
  }
  void spawn() {

    scene_.create_default_player(player_name, w_);

    //console.load_settings();
    int draw_dist = 512;//get_cvar("r_view_distance");

    wv_ = new world_view(w_, scene_.get_player(0)->get_position(), draw_dist);
    wv_->update_occlusion();
    wv_->initialize_meshes();
    timer::set_start();
    scene_.get_player(0)->spawn(/*player::flags::SPAWN_ON_SURFACE |
                   player::flags::SPAWN_RANDOM_XY*/);

    timer::set_start();
  }
  void quit() {
    //	player0->save();
    wv_->get_world()->save_all();
    delete wv_->get_world();
    delete wv_;
    exit();
  }
  void die() {
    printf("you died.\n");
    quit();
  }

private:
  void tickEvent() override {
  /*  if (!player0->is_alive()) {
      die();
    }*/
    scene_.update(timer::step());
    track_player();
    wv_->update_occlusion();
    wv_->queue_update_stale_meshes();
    wv_->remesh_from_queue();
  }
  
  void drawEvent() override {
    block_pass->set_scene(&scene_).set_player(scene_.get_player(0)).draw_world_view(wv_);
    swapBuffers();
    timer::next();
    return;
  }
  void mouseMoveEvent(MouseMoveEvent &event) override {
    scene_.get_player(0)->mousemove(event.relativePosition().x(),
                                     event.relativePosition().y());
  }

  void mouseReleaseEvent(MouseEvent &event) override {
    scene_.get_player(0)->mouseup(
        event.button() == MouseEvent::Button::Left
            ? 0
            : event.button() == MouseEvent::Button::Right ? 1 : 2);
  }
  void mousePressEvent(MouseEvent &event) override {
    player *player0 = scene_.get_player(0);
    player0->mousedown(event.button() == MouseEvent::Button::Left
                           ? 0
                           : event.button() == MouseEvent::Button::Right ? 1
                                                                         : 2);
  }
  void keyPressEvent(KeyEvent &event) override {
    player *player0 = scene_.get_player(0);
    if (event.key() == KeyEvent::Key::Esc) {
      quit();
    }
    player0->keydown(event.key());
  }
  void keyReleaseEvent(KeyEvent &event) override {
    player *player0 = scene_.get_player(0);
    player0->keyup(event.key());
  }
};
MAGNUM_APPLICATION_MAIN(game)
