#include <glm/glm.hpp>
#include <QMouseEvent>
#include "tool/movement.hpp"
#include "primitive/plane.hpp"
#include "primitive/ray.hpp"
#include "state.hpp"
#include "camera.hpp"
#include "intersection.hpp"
#include "view/properties/movement.hpp"

struct ToolMovement::Impl {
  glm::vec3 position;

  bool moveOnPlane (const glm::vec3& normal, const glm::uvec2& p) {
    const PrimRay   ray   = State::camera ().ray (p);
    const PrimPlane plane (this->position, normal);
          float     t;
    
    if (IntersectionUtil::intersects (ray, plane, t)) {
      this->position = ray.pointAt (t);
      return true;
    }
    return false;
  }

  bool moveOnPlane (unsigned int normalIndex, const glm::uvec2& p) {
    glm::vec3 normal (0.0f);
    normal[normalIndex] = 1.0f;
    return this->moveOnPlane (normal, p);
  }

  bool moveOnNormal (unsigned int normalIndex, const glm::uvec2& p) {
    glm::vec3 normal    = State::camera ().toEyePoint ();
    normal[normalIndex] = 0.0f;

    const PrimRay   ray   = State::camera ().ray (p);
    const PrimPlane plane (this->position, glm::normalize (normal));
          float     t;
    
    if (IntersectionUtil::intersects (ray, plane, t)) {
      this->position[normalIndex] = ray.pointAt (t)[normalIndex];
      return true;
    }
    return false;
  }

  bool moveOnXY     (const glm::uvec2& p) { return this->moveOnPlane  (2, p); }
  bool moveOnYZ     (const glm::uvec2& p) { return this->moveOnPlane  (0, p); }
  bool moveOnXZ     (const glm::uvec2& p) { return this->moveOnPlane  (1, p); }
  bool moveAlongX   (const glm::uvec2& p) { return this->moveOnNormal (0, p); }
  bool moveAlongY   (const glm::uvec2& p) { return this->moveOnNormal (1, p); }
  bool moveAlongZ   (const glm::uvec2& p) { return this->moveOnNormal (2, p); }

  bool moveOnCamera (const glm::uvec2& p) { 
    return this->moveOnPlane (glm::normalize (State::camera ().toEyePoint ()), p); 
  }

  bool byMouseEvent (ViewPropertiesMovement* properties, QMouseEvent* e) {
    if (e->buttons () == Qt::LeftButton) {
      glm::uvec2 p (e->x (), e->y ());

      if (e->modifiers ().testFlag (Qt::ShiftModifier) ) {
        if      (properties->x ()) { return this->moveAlongX (p); }
        else if (properties->y ()) { return this->moveAlongY (p); }
        else if (properties->z ()) { return this->moveAlongZ (p); }
      }
      else {
        return this->byScreenPos (properties, p);
      }
    }
    return false;
  }

  bool byScreenPos (ViewPropertiesMovement* properties, const glm::uvec2& p) {
    if      (properties->xy     ()) { return this->moveOnXY     (p); }
    else if (properties->yz     ()) { return this->moveOnYZ     (p); }
    else if (properties->xz     ()) { return this->moveOnXZ     (p); }
    else if (properties->camera ()) { return this->moveOnCamera (p); }
    else assert (false);
  }
};

DELEGATE_BIG6  (ToolMovement)
GETTER_CONST   (const glm::vec3&, ToolMovement, position)
SETTER         (const glm::vec3&, ToolMovement, position)
DELEGATE2      (bool            , ToolMovement, byMouseEvent, ViewPropertiesMovement*, QMouseEvent*)
DELEGATE2      (bool            , ToolMovement, byScreenPos , ViewPropertiesMovement*, const glm::uvec2&)