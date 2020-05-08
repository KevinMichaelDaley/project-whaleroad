#include "common/timer.h"
#include "gfx/shader.h"
#include "gfx/chunk_mesh.h"
#include "phys/player.h"
#include "vox/world.h"
#include "ui/wrap.h"
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
#include <Magnum/ImGuiIntegration/Widgets.h>
#include <Corrade/Utility/Resource.h>

#include <Corrade/Utility/Debug.h>
#include <chrono>  
#include <cstdio>
#include <array>
#include <set>
#include <map>
#include <unistd.h>
using namespace Magnum;
class editor;
class EditorUI: public UI{
public:
    std::vector<std::string> command_history;
    int hist_ptr;
    std::string command;
    char InputBuf[256];
    GL::Texture2D* atlas, *toolbox;
    EditorUI(editor* editor, GL::Texture2D* atlas_, GL::Texture2D* toolbox_):
        UI((Platform::Application*)editor){
            command="";
            hist_ptr=0;
            toolbox=toolbox_;
            atlas=atlas_;
            std::memset(InputBuf, 0, sizeof(InputBuf));
    }
    static void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    void tool_button(int i, int N, std::string cmd, std::string desc="", GL::Texture2D* tex=nullptr){
            if(desc==""){
                desc=cmd;
            }
            if(tex==nullptr){
                tex=toolbox;
            }
        
            
            if (ImGuiIntegration::imageButton(*tex,{32,32},{{i/(float)N, 0}, {(i+1)/(float)N, 0}})){
                command=cmd;
                submit_cmd();
            }
            HelpMarker(desc.c_str());
    }
    virtual void ui_loop() override{
        bool p_open;
        if (ImGui::Begin("ToolBox", &p_open))
        {
            
            std::vector<std::string> cmds={"pick","set","+select","box","clear","yank","paste","fill","cursor","mark", "recall", "save"};
            for(int i=0; i<cmds.size(); ++i){
                tool_button(i,cmds.size(),cmds[i]);
            }
            
        }
        ImGui::End();
        if (ImGui::Begin("Palette", &p_open))
        {
            for(int i=AIR; i<NUM_BLOCK_TYPES; ++i){
                std::stringstream cmd;
                cmd<<"block ";
                cmd<<i;
                tool_button(i, 256, cmd.str(), "", atlas);
            }
        }
        ImGui::End();
    }
    virtual void submit_cmd(){
        command_history.push_back(command);
        command="";
    }
    virtual std::string fetch_cmd(){
        if(command_history.size()>0){
            hist_ptr++;
            if(hist_ptr>command_history.size()){
                command_history.clear();
            }
            return command_history[hist_ptr-1];
        }
        return "";
    }
    virtual bool is_viewport_in_focus(){return true;}
};
class EditTool{
public:
    typedef Magnum::Platform::Sdl2Application::KeyEvent::Key keycode_t;
    virtual bool is_nav(){return true;}
    virtual void click(editor* editor, int x, int y){}
    virtual void drag(editor* editor, int x, int y){}
    virtual void keydown(keycode_t key){}
    virtual void keyup(keycode_t key){}
};

class editor : public Platform::Sdl2Application {

friend EditTool;
friend EditorUI;
private:
  block_t place_block;
  bool mouse_look;
  block_default_forward_pass* block_pass;
  world_view *wv_;
  EditorUI* ui;
  job_pool* jobs;
  int draw_dist;
  world* w_;
  scene scene_;
  std::string player_name;
  GL::Texture2D atlas, toolbox;
  char** argv; int argc;
  int pos_x, pos_y;
  std::vector<EditTool*> ToolBox;
  std::set<std::array<int,3>> selection;
  
  std::string command;
  int cursor_x, cursor_y, cursor_z;
  bool ctrl, alt, shift;
  int cursor[3];
  bool saved_cursor;
  std::map<std::string, std::array<int,3>> cursors;
  std::array<int,3> last_cursor;
public:
  
  explicit editor(const Arguments &arguments)
    
      :  Platform::Sdl2Application(
                                  arguments,
                                  Configuration{}.setSize({2560,1440})
                                  , GLConfiguration{}.setVersion(GL::Version::GL330)   
                                  ) ,
          argc(arguments.argc),
          argv(arguments.argv),
          ui(nullptr),
          saved_cursor(false),
          command("")
      {
    mouse_look=false;
    place_block=AIR;
    setSwapInterval(0);
    setMouseLocked(true);
    PluginManager::Manager<Trade::AbstractImporter> manager;
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
    if(!importer->openData(rs.getRaw("toolbox.tga"))){
        std::exit(2);
    }
    image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    toolbox.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setMinificationFilter(GL::SamplerFilter::Nearest)
        .setStorage(1, GL::TextureFormat::RGB8, image->size())  
        .setSubImage(0, {}, *image)
        .generateMipmap();  
    block_pass=new block_default_forward_pass(atlas);
    jobs=new job_pool(constants::MAX_CONCURRENCY,w_);
    load_world("world",0,0,-1);
    ui=new EditorUI(this,&atlas,&toolbox);
    spawn();
  }
  void load_world(std::string name, int x, int y, int z=-1) {
    bool new_world;
    w_ = world::load_or_create(name, new_world);
    if(z<0){
        z=w_->get_z(x,y)+1;
    }
    scene_.create_editor_player("viewer", w_, x,y,z);
  }
  void save_world(){
    w_->save_all();
  }
  void track_player() {
    player *player0 = scene_.get_player(0);
    Vector3 v = player0->get_position();
    wv_->update_center(v);
  }
  void spawn() {
    
    
    
    
    
    

    //console.load_settings();
    draw_dist =  96;//get_cvar("r_view_distance");
    
    wv_=new world_view{w_,scene_.get_player(0)->get_position(), draw_dist, jobs};
    world_view::update_occlusion_radius(wv_,draw_dist);
    wv_->initialize_meshes();
    

    timer::set_start();
  }
  void quit() {
    //	player0->save();
    wv_->get_world()->save_all();
    delete wv_->get_world();
    delete wv_;
    std::terminate();
  }

private:
  void tickEvent() {
  /*  if (!player0->is_alive()) {
      die();
    }*/
    scene_.update(std::min(timer::step(),0.06));
    if(rand()%100==0){
        Debug{}<<timer::step()<<Utility::Debug::newline;
    }
    wv_->queue_update_stale_meshes();
    wv_->add_remesh_jobs();
    track_player();
    redraw();
  } 
  
  virtual void viewportEvent(ViewportEvent& event) override {
      //if(ui) ui->viewportEvent(event);
  }

  virtual void drawEvent() override {
    
    
    tickEvent();
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    block_pass->set_scene(&scene_).set_player(scene_.get_player(0)).draw_world_view(wv_);
    if(ui) ui->drawEvent();
    
    swapBuffers();
    timer::next();
    return;
  }
  bool is_scene_in_focus(){
      return ui->is_viewport_in_focus();
  }
  void mouseMoveEvent(MouseMoveEvent &event) override {
      
        if(ui) ui->mouseMoveEvent(event);
    
        scene_.get_player(0)->mousemove(event.relativePosition().x(),
                                            event.relativePosition().y());
  }


  void mouseReleaseEvent(MouseEvent &event) override {
    
    
    if(ui) ui->mouseReleaseEvent(event);
   
    
    
    
    
    
  }
  void mousePressEvent(MouseEvent &event) override {
    if(ui) if(!ui->mousePressEvent(event)){
        pos_x=event.position().x();
        pos_y=event.position().y();
        interpret_command(ui->fetch_cmd());
    }
    
    
  }
  
  void keyPressEvent(KeyEvent &event) override {
    if(ui) ui->keyPressEvent(event);
    typedef player::keycode_t keycode;
    if(event.key()==keycode::Down){
        interpret_command(alt?"n":"j");
    }
    if(event.key()==keycode::Up){
        interpret_command(alt?"u":"k");
    }
    if(event.key()==keycode::Left){
        interpret_command("h");
    }
    if(event.key()==keycode::Right){
        interpret_command("l");
    }
    if(event.key()==keycode::LeftAlt || event.key()==keycode::RightAlt){
        alt=true;
    }
    
    if(event.key()==keycode::LeftShift || event.key()==keycode::RightShift){
        shift=true;
    }
    
    if(event.key()==keycode::LeftCtrl || event.key()==keycode::RightCtrl){
        ctrl=true;
    }
    scene_.get_player(0)->keydown(event.key());
    
    
  }
  void keyReleaseEvent(KeyEvent &event) override {
      typedef player::keycode_t keycode;
    if(ui) ui->keyReleaseEvent(event);    
    if(event.key()==keycode::LeftAlt || event.key()==keycode::RightAlt){
        alt=false;
    }
    
    if(event.key()==keycode::LeftShift || event.key()==keycode::RightShift){
        shift=false;
    }
    
    if(event.key()==keycode::LeftCtrl || event.key()==keycode::RightCtrl){
        ctrl=false;
    }
    if(!isTextInputActive())
        scene_.get_player(0)->keyup(event.key());
    
  }
  void select(int x, int y, int z, bool remove=false){
        if(z<0 || z>=constants::WORLD_HEIGHT){
            return;
        }
        if(!remove)
            selection.insert({x,y,z});
        else
            deselect(x,y,z);
  }
  void select_rect(int x0, int y0, int z0, int x1, int y1, int z1, bool remove=false){
      for(int x=std::min(x0,x1); x<std::max(x0,x1); ++x){
        for(int y=std::min(y0,y1); y<std::max(y0,y1); ++y){
            for(int z=std::min(z0,z1); z<std::max(z1,z0); ++z){
                select(x,y,z,remove);
            }
        }
      }
  }
  void deselect(int x, int y, int z){
              selection.erase({x,y,z});
  }
  void select_flood_fill(int x, int y, int z, bool remove=false, int max_iter=256){
      int vox=w_->get_voxel(x,y,z);
      select(x,y,z,remove);
      bool have_more=true;
      for(int it=0; it<max_iter && have_more; ++it){
          have_more=false;
          for(std::array<int,3> p: selection){
            for(int dx=-1; dx<=2; ++dx){
                for(int dy=-1; dy<=2; ++dy){
                    for(int dz=-1; dz<=2; ++dz){
                        if(dz==0 && dy ==0 && dz==0) continue;
                        if(p[2]+dz>0 && p[2]+dz <= constants::WORLD_HEIGHT){
                            if(w_->get_voxel(p[0]+dx,p[1]+dy,p[2]+dz)==vox){
                                have_more=true;
                                select(p[0]+dx,p[1]+dy,p[2]+dz,remove);
                            }
                        }
                    }
                }
            }
          }
      }
                                
                  
  }
  void intersect_rect_selection(int x0, int y0, int z0, int x1, int y1, int z1){
      for(std::array<int,3> p: selection){
          if(p[0]<x0 || p[0]>=x1 || p[1]<y0 || p[1]>=y1 || p[2]<z0 || p[2]>=z1){
             deselect(p[0],p[1],p[2]);
          }
      }
  }
  void find_selection_min(int& xm, int& ym, int& zm){
      xm=ym=std::numeric_limits<int>::max();
      zm=constants::WORLD_HEIGHT;
      for(std::array<int,3> p: selection){
          xm=std::min(xm,p[0]);
          ym=std::min(ym, p[1]);
          zm=std::min(ym, p[2]);
      }
  }
  void find_selection_max(int& xm, int& ym, int& zm){
      xm=ym=std::numeric_limits<int>::min();
      zm=0;
      for(std::array<int,3> p: selection){
          xm=std::max(xm,p[0]);
          ym=std::max(ym, p[1]);
          zm=std::max(ym, p[2]);
      }
  }
  void toggle_physics(){
      scene_.get_player(0)->toggle_physics();
  }
  void yank(FILE* fp){
      char sel_block[12];
      int xm,ym,zm;
      find_selection_min(xm,ym,zm);
      for(std::array<int,3> p: selection){
          uint32_t px=uint32_t(p[0]-xm);
          sel_block[0]=px&255u;
          sel_block[1]=(px>>8u)&255u;
          sel_block[2]=(px>>16u)&255u;
          sel_block[3]=(px>>24u)&255u;
          uint32_t py=uint32_t(p[1]-ym);
          sel_block[4]=py&255;
          sel_block[5]=(py>>8u)&255u;
          sel_block[6]=(py>>16u)&255u;
          sel_block[7]=(py>>24u)&255u;
          uint32_t pz=uint32_t(p[1]-ym);
          sel_block[8]=pz&255u;
          sel_block[9]=(pz>>8u)&255u;
          uint16_t pb=uint16_t(std::abs(w_->get_voxel(p[0],p[1],p[2])));
          sel_block[10]=pb&255u;
          sel_block[11]=(pb>>8u)&255u;
          fwrite(sel_block, 12,1,fp);
      }
  }
  bool is_selected(int x, int y, int z){
      return selection.find({x,y,z})!=selection.end();
  }
  void paste(FILE* fp, int x, int y, int z, bool paste_empty=false, uint16_t subtract=0){
        uint8_t sel_block[12];
        int xm,ym,zm;
        find_selection_min(xm,ym,zm);
        while(fread(sel_block,12,1,fp)==12){
            uint32_t x=sel_block[0]+sel_block[1]<<8u+sel_block[2]<<16u+sel_block[3]<<24u;
            uint32_t y=sel_block[4]+sel_block[5]<<8u+sel_block[6]<<16u+sel_block[7]<<24u;
            uint16_t z=sel_block[7]+sel_block[8]<<8u;
            uint16_t b=sel_block[9]+sel_block[10]<<8u;
            if(selection.empty() || is_selected(x+xm,y+ym,z+zm)){
                if((b>AIR || paste_empty) && !subtract){
                    w_->set_voxel(x,y,z,b);
                }
                else if(b>AIR && subtract>0){
                    w_->set_voxel(x,y,z,subtract);
                }
            }
        }
  }
  void yank(std::string fname){
      FILE* fp=fopen(fname.c_str(),"r");
      yank(fp);
      fclose(fp);
  }
  void paste(std::string fname,int x, int y, int z, bool paste_empty=false, uint16_t subtract=0){
      FILE* fp=fopen(fname.c_str(),"w+");
      paste(fp,x,y,z,paste_empty,subtract);
      fclose(fp);
  }
  void fill(block_t b){
      for(std::array<int,3> p: selection){
          w_->set_voxel(p[0],p[1],p[2], b);
      }
  }
  void interpret_command(std::string cmd){
      std::vector<std::string> command_args=split_string(cmd,' ');
      if(command_args.size()==0){
          return;
      }
      if(command_args[0]=="block"){
        if(command_args.size()>1){
            place_block=atoi(command_args[1].c_str());
        }
        else{
            place_block++;
            if(place_block==NUM_BLOCK_TYPES){
                place_block=AIR;
            }
        }
      }
      if(command_args[0]=="u"){
          if(command_args.size()==1) command_args.push_back("1");
          cursor[2]+=atoi(command_args[1].c_str());
          if(cursor[2]>=constants::WORLD_HEIGHT){
              cursor[2]=constants::WORLD_HEIGHT-1;
          }
      }
      if(command_args[0]=="n"){
          if(command_args.size()==1) command_args.push_back("1");
          cursor[2]-=atoi(command_args[1].c_str());
          if(cursor[2]<0){
              cursor[2]=0;
          }   
      }
      if(command_args[0]=="h"){
          if(command_args.size()==1) command_args.push_back("1");
          cursor[0]-=atoi(command_args[1].c_str());
      }
      if(command_args[0]=="j"){
          if(command_args.size()==1) command_args.push_back("1");
          cursor[1]-=atoi(command_args[1].c_str());
      }
      if(command_args[0]=="k"){
          if(command_args.size()==1) command_args.push_back("1");
          cursor[1]+=atoi(command_args[1].c_str());
      }
      if(command_args[0]=="l"){
          if(command_args.size()==1) command_args.push_back("1");
          cursor[0]+=atoi(command_args[1].c_str());
      }
      if(command_args[0]=="place"){
          w_->set_voxel(cursor[0],cursor[1],cursor[2],place_block);
      }
      if(command_args[0]=="pick"){
           place_block=w_->get_voxel(cursor[0],cursor[1],cursor[2]);
      }
      if(command_args[0]=="+select" || command_args[0]=="+s"){
        select(cursor[0],cursor[1],cursor[2]);
      }
      if(command_args[0]=="&select" || command_args[0]=="&s"){
          selection.clear();
          select(cursor[0],cursor[1],cursor[2]);
      }
      if(command_args[0]=="deselect" || command_args[0]=="-s"){
            select(atoi(command_args[1].c_str()), atoi(command_args[2].c_str()), atoi(command_args[3].c_str()), true);
      }
      if(command_args[0]=="clr" || command_args[0]=="clear"){
          selection.clear();
      }
      if(command_args[0]=="bounds"){
            int x0,y0,z0,x1,y1,z1;
            if(!selection.empty()){
                find_selection_min(x0,y0,z0);
                find_selection_max(x1,y1,z1);
                select_rect(x0,y0,z0,x1,y1,z1,false);
            }
      }
      if(command_args[0]=="|box" || command_args[0]=="box"){
          std::array<int,3> cursor0;
          if(command_args.size()>1){
              if(cursors.find(command_args[1])!=cursors.end()){
                  cursor0=cursors[command_args[1]];
                  select_rect(cursor0[0],cursor0[1],cursor0[2],cursor[0],cursor[1],cursor[2],false);
              }
          }
          else if(saved_cursor){
              cursor0=last_cursor;
              select_rect(cursor0[0],cursor0[1],cursor0[2],cursor[0],cursor[1],cursor[2],false);
          }
      }
      if(command_args[0]=="-box"){
          std::array<int,3> cursor0;
          if(command_args.size()>1){
              if(cursors.find(command_args[1])!=cursors.end()){
                  cursor0=cursors[command_args[1]];
                  select_rect(cursor0[0],cursor0[1],cursor0[2],cursor[0],cursor[1],cursor[2],true);
              }
          }
          else if(saved_cursor){
              cursor0=last_cursor;
              select_rect(cursor0[0],cursor0[1],cursor0[2],cursor[0],cursor[1],cursor[2],true);
          }
      }         
      if(command_args[0]=="&box"){
          std::array<int,3> cursor0;
          if(command_args.size()>1){
              if(cursors.find(command_args[1])!=cursors.end()){
                  cursor0=cursors[command_args[1]];
                  intersect_rect_selection(cursor0[0],cursor0[1],cursor0[2],cursor[0],cursor[1],cursor[2]);
              }
          }
          else if(saved_cursor){
              cursor0=last_cursor;
              intersect_rect_selection(cursor0[0],cursor0[1],cursor0[2],cursor[0],cursor[1],cursor[2]);
          }
      }
                  
              
      if(command_args[0]=="yank" || command_args[0]=="y"){
          if(command_args.size()==1){
              command_args.push_back("paste_buf");
          }
          yank(command_args[1]);
      }
      if(command_args[0]=="paste"){
          if(command_args.size()==1){
              command_args.push_back("paste_buf");
          }
          paste(command_args[1],cursor[0],cursor[1],cursor[2],true);
      }
      if(command_args[0]=="brush"){
          if(command_args.size()==1){
              command_args.push_back("paste_buf");
          }
          paste(command_args[1],cursor[0],cursor[1],cursor[2],false);
      }
      if(command_args[0]=="carve"){
          if(command_args.size()==1){
              command_args.push_back("paste_buf");
          }
          paste(command_args[1],cursor[0],cursor[1],cursor[2],false,AIR);
      }
      if(command_args[0]=="fill"){
          fill(place_block);
      }
      if(command_args[0]=="similar"){
          for(std::array<int,3> p: selection){
            select_flood_fill(p[0],p[1],p[2]);
          }
      }
      if(command_args[0]=="move"){
            scene_.get_player(0)->set_position(cursor[0],cursor[1],cursor[2]);              
      }
      if(command_args[0]=="cursor"){
            if(scene_.get_player(0)->raycast(pos_x,pos_y,draw_dist)){
                cursor[0]=scene_.get_player(0)->target[0];
                cursor[1]=scene_.get_player(0)->target[1];
                cursor[2]=scene_.get_player(0)->target[2];
            }
      }
      if(command_args[0]=="mark"){
          if(command_args.size()==1){
              command_args.push_back("p0");
          }
          cursors[command_args[1]]={cursor[0],cursor[1],cursor[2]};
          last_cursor=cursors[command_args[1]];
          saved_cursor=true;
      }
      if(command_args[0]=="recall"){
          if(command_args.size()==1){
              if(saved_cursor){
                std::array<int,3> last_cursor2=last_cursor;
                last_cursor={cursor[0], cursor[1],cursor[2]};
                cursor[0]=last_cursor2[0];
                cursor[1]=last_cursor2[1];
                cursor[2]=last_cursor2[2];
                
              }
          }
          else{
              cursor[0]=cursors[command_args[1]][0];
              cursor[1]=cursors[command_args[1]][1];
              cursor[2]=cursors[command_args[1]][2];
          }
      }
      if(command_args[0]=="write"){
          save_world();
      }
      if(command_args[0]=="physics"){
          toggle_physics();
      }
  }
      
};
#ifdef EDITOR

MAGNUM_APPLICATION_MAIN(editor)
#endif
