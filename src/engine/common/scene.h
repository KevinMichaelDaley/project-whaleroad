#pragma once
#include "entity.h"
#include "phys/player.h"
#include <vector>


class scene{
public:
    std::vector<entity*> entities;
    std::vector<player*> players;
    void create_default_player(std::string player_name, world* w);
    player* get_player(int id);
    entity* get_entity(std::string name);
    void update(float step);
};
        
