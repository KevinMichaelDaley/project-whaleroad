
/*#include <Magnum/Magnum.h>

struct plane_t{
    Vector3 n;
    float d;
};
bool does_box_intersect_sphere(Range3D box, Vector3 sphere_center,
                               float sphere_radius) {
  float dist_squared = R * R;
  Vector3 bbL = box.bottomBackLeft(), tfR = box.topFrontRight();
  if (sphere_center.x() < bbL.x())
    dist_squared -= squared(sphere_center.x() - bbL.x());
  else if (sphere_center.x() > tfR.y())
    dist_squared -= squared(sphere_center.x() - tfR.x());
  if (sphere_center.y() < bbL.y())
    dist_squared -= squared(sphere_center.y() - bbL.y());
  else if (sphere_center.y() > tfR.y())
    dist_squared -= squared(sphere_center.y() - tfR.y());
  if (sphere_center.z() < bbL.z())
    dist_squared -= squared(sphere_center.z() - bbL.z());
  else if (sphere_center.z() > tfR.z())
    dist_squared -= squared(sphere_center.z() - tfR.z());
  return dist_squared > 0;
}

bool is_chunk_outside_frustum(int x0, int y0, std::array<plane_t, 6>
frustumPlanes) { plane_t absFrustumPlanes[6]; for (unsigned int plane = 0; plane
< 6; ++plane) { absFrustumPlanes[plane].n =
Vector3{std::abs(frustumPlanes[plane].n.x()),
                                        std::abs(frustumPlanes[plane].n.y()),
                                        std::abs(frustumPlanes[plane].n.z())};
  }

  for (unsigned int plane = 0; plane < 6; ++plane) {
    bool all_wrong = true;
    for (int i = 0; (i <= CHUNK_WIDTH) && all_wrong; i += CHUNK_WIDTH) {
      for (int j = 0; (j <= CHUNK_WIDTH) && all_wrong; j += CHUNK_WIDTH) {
        for (int k = 0; k <= WORLD_HEIGHT; k += WORLD_HEIGHT) {
          if (absFrustumPlanes[plane].n.x() * (i + x0) +
                  absFrustumPlanes[plane].n.y() * (j + y0) +
                  absFrustumPlanes[plane].n.z() * (k) >
              0.0) {
            all_wrong = false;
            break;
          }
        }
      }
    }
    if (all_wrong) {
      return true;
    }
  }
  return false;
}
*/
