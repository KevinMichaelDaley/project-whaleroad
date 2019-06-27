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

#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include "Magnum/GL/RenderbufferFormat.h"
#include "Magnum/ImageView.h"
#include <Corrade/Utility/Resource.h>

#include <Corrade/Utility/Debug.h>
#include <chrono>   

#ifndef __ANDROID__

#include <Magnum/Platform/Sdl2Application.h>
#else

#include <Magnum/Platform/AndroidApplication.h>
#endif
using namespace Magnum;

#ifndef __ANDROID__
class game : public Platform::Sdl2Application {
#else
#define TOUCH_CONTROLS 1
class game : public Platform::AndroidApplication {
#endif
private:
  block_default_forward_pass* block_pass;
  world_view *wv_;
  world* w_;
  scene scene_;
  std::string player_name;
  GL::Texture2D atlas;
  char** argv; int argc;
  double touch_time;
  bool touch;
  int pos_x, pos_y;
public:
  explicit game(const Arguments &arguments)
    #ifndef __ANDROID__
      :  Platform::Sdl2Application(
                                  arguments,
                                  Configuration{}.setWindowFlags(Magnum::Platform::Sdl2Application::Configuration::WindowFlag::Fullscreen | Magnum::Platform::Sdl2Application::Configuration::WindowFlag::MouseLocked).setSize({2560,1080})
                                  ) ,
          argc(arguments.argc),
          argv(arguments.argv)
      {
    setSwapInterval(0);
    setMouseLocked(true);  
#else
    :  Platform::AndroidApplication(
                                  arguments
                                  
                                  ) 
      {
#endif  
    touch=false;
    touch_time=0.0;
    PluginManager::Manager<Trade::AbstractImporter> manager;
#ifndef __ANDROID__
    player_name = arguments.argc > 1 ? arguments.argv[1] : "new_player";
#else
    player_name = "new_player";
#endif
    std::unique_ptr<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("TgaImporter");  
    if(!importer) std::exit(1);
    const Utility::Resource rs{"art"};
    if(!importer->openData(rs.getRaw("atlas.tga"))){
        std::exit(2);
    }           
    
    GL::Renderer::setClearColor({0.08,0.069,0.07,1.0});
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    atlas.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setMinificationFilter(GL::SamplerFilter::Nearest)
        .setStorage(1, GL::TextureFormat::RGB8, image->size())  
        .setSubImage(0, {}, *image)
        .generateMipmap();  
    block_pass=new block_default_forward_pass(atlas);
    load_world("world");
    int x,y,z;
    spawn();
    
    
  }
  void load_world(std::string name) {
    bool new_world;
    w_ = world::load_or_create(name, new_world);
    if (new_world) {
      w_->save_all();
    }
    
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
    camera cam;
    cam.look_at({0,0,0},{0,0,1},{0,1,0});
    cam.set_perspective(1.0,1.0,0.1,128.0);
    Matrix4 viewproj=cam.projection*cam.view;
    printf("[");
    for(int i=0; i<4; ++i){
        printf("[");
        for(int j=0; j<4; ++j){
            printf("%f",viewproj[j][i]);
            if(j<3) printf(",");
        }
        printf("]\n");
    }
    printf("]\n\n\n");
    
    
    
    
    cam.look_at({0,0,0},{0,1,0},{0,0,1});
    cam.set_perspective(1.0,1.0,0.1,128.0);
    viewproj=cam.projection*cam.view;
    printf("[");
    for(int i=0; i<4; ++i){
        printf("[");
        for(int j=0; j<4; ++j){
            printf("%f",viewproj[j][i]);
            if(j<3) printf(",");
        }
        printf("]\n");
    }
    printf("]\n\n\n");
    scene_.create_default_player(player_name, w_);
    
    
    
    cam.look_at({0,0,0},{1,0,0},{0,0,1});
    cam.set_perspective(1.0,1.0,0.1,128.0);
    viewproj=cam.projection*cam.view;
    printf("[");
    for(int i=0; i<4; ++i){
        printf("[");
        for(int j=0; j<4; ++j){
            printf("%f",viewproj[j][i]);
            if(j<3) printf(",");
        }
        printf("]\n");
    }
    printf("]\n\n\n");
    scene_.create_default_player(player_name, w_);

    //console.load_settings();
    int draw_dist = 96 ;//get_cvar("r_view_distance");
    wv_ = new world_view(w_, scene_.get_player(0)->get_position(), draw_dist);
    wv_->initialize_meshes();
    wv_->update_occlusion(draw_dist);
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
    std::terminate();
  }
  void die() {
    Debug{}<<"you died.\n";
    quit();
  }

private:
  void tickEvent() 
#ifndef __ANDROID__
    override {
#else
    {
#endif
  /*  if (!player0->is_alive()) {
      die();
    }*/
#ifndef __ANDROID__
    touch=false;
#endif  
    if(touch){
        touch_time+=timer::step();
    }
    scene_.update(std::min(timer::step(),0.06));
    if(rand()%100==0){
        Debug{}<<timer::step()<<Utility::Debug::newline;
    }
    track_player();
    wv_->queue_update_stale_meshes();
    wv_->remesh_from_queue();
    redraw();
  } 
  virtual void drawEvent() override {
#ifdef __ANDROID__
      tickEvent();
#endif
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    block_pass->set_scene(&scene_).set_player(scene_.get_player(0)).draw_world_view(wv_);
    swapBuffers();
    timer::next();
    return;
  }
  void mouseMoveEvent(MouseMoveEvent &event) override {
#ifndef TOUCH_CONTROLS
    scene_.get_player(0)->mousemove(event.relativePosition().x(),
                                     event.relativePosition().y());
#else
    
    if(event.position().x()>windowSize().x()/2){
        scene_.get_player(0)->mousemove((event.position().x()-pos_x)*0.1,
                                     (event.position().y()-pos_y)*0.1);
    }
    else{
        scene_.get_player(0)->strafe(event.position().x()-pos_x<0, (event.position().x()-pos_y*0.0005));
        scene_.get_player(0)->pedal((event.position().y()-pos_y<0), (event.position().y()-pos_y*0.0005));
        
    }
    pos_y=event.position().x(); 
    pos_x=event.position().y();
    touch_time=0.0;
#endif
  }


  void mouseReleaseEvent(MouseEvent &event) override {
      
#ifndef TOUCH_CONTROLS
    scene_.get_player(0)->mouseup(
        event.button() == MouseEvent::Button::Left
            ? 0
            : event.button() == MouseEvent::Button::Right ? 1 : 2);
#else
    touch=false;
    if(touch_time){
        
        scene_.get_player(0)->mousedown(touch_time<0.9? 0
                            :  1);
        touch_time=0.0;
    }
#endif
    
  }
  void mousePressEvent(MouseEvent &event) override {
    player *player0 = scene_.get_player(0);
#ifndef TOUCH_CONTROLS
    
    pos_y=event.position().x();
    pos_x=event.position().y();
    player0->mousedown(event.button() == MouseEvent::Button::Left
                           ? 0
                           : event.button() == MouseEvent::Button::Right ? 1
                                                                         : 2);
#else
    touch=true;
#endif
    
  }
#ifndef TOUCH_CONTROLS
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
#endif

};
#ifndef __ANDROID__
MAGNUM_APPLICATION_MAIN(game)
#else
MAGNUM_ANDROIDAPPLICATION_MAIN(game)
#endif
