#include "scene.h"
#include <cassert>
    void scene::create_default_player(std::string player_name, world* w){
        player* p=new player(player_name, w);
        entities.push_back(p);
        players.push_back(p);
    }
    player* scene::get_player(int id){
        return players[id];
    }
    entity* scene::get_entity(std::string name){
        assert(name!="");
        for(int i=0, e=entities.size(); i<e; ++i){
            entity* ent=entities[i];
            if(ent->get_name()==name){
                return ent;
            }
        }
        return nullptr;
    }
    void scene::update(float step){
        for(int i=0, e=entities.size(); i<e; ++i){
            entities[i]->update(step);
        }
    }
