#include "camera.h"

#include <Magnum/GL/AbstractShaderProgram.h>
class drawable {
public:
  virtual bool is_visible(camera *cam) = 0;
  virtual void draw(Magnum::GL::AbstractShaderProgram *program) = 0;
};
