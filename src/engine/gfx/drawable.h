#include "camera.h"
class shader;
#include <Magnum/GL/AbstractShaderProgram.h>
class drawable {
public:
  virtual bool is_visible(camera *cam) = 0;
  virtual void draw(shader *program, camera *cam) = 0;
};
