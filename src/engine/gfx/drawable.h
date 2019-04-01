#include "camera.h"
class drawable {
public:
  virtual bool is_visible(camera *cam) = 0;
  virtual void draw(GL::AbstractShaderProgram *program) = 0;
};
