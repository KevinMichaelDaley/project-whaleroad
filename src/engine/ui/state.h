
class state_manager{
    enum e_state{
        ST_MENU,
        ST_LOAD,
        ST_RUN,
        ST_EDIT,
        ST_INV,
        ST_MAP,
        ST_CHAR,
        ST_PAUSE,
    } st,st0;
    virtual bool is_player_input(){
        if(st==ST_RUN){
            return true;
        }
        return false;
    }
    
    virtual bool is_in_game(){
        return st!=ST_MENU && st!=ST_LOAD;
    }
    virtual void pause(){
        st0=st;
        st=ST_PAUSE;
    }
    virtual void resume(){
        if(st==ST_PAUSE || st==ST_INV || st==ST_MAP || st==ST_CHAR){
            st=st0;
        }
    }
    virtual void launch(bool edit){
        if(!is_in_game()){
            st=edit? ST_EDIT : ST_RUN;
        }
    }
    virtual void load(bool edit){
        if(!is_in_game()){
            st=ST_LOAD;
        }
    }
    virtual void menu(){
        st=ST_MENU;
    }
    virtual void inv(is_in_game()){
        if(is_in_game()){
            st=ST_INV;
        }
    }
    virtual void map(){
         if(is_in_game()){
            st=ST_MAP;
        }
    }
    virtual void journal(){
         if(is_in_game()){
            st=ST_CHAR;
        }
    }
};
