#include <unordered_set>
#include <set>
#include <glm/glm.hpp>
#include "action/carve.hpp"
#include "macro.hpp"
#include "id.hpp"
#include "action/unit.hpp"
#include "action/subdivide.hpp"
#include "partial-action/modify-vertex.hpp"
#include "sphere.hpp"
#include "carve-brush.hpp"
#include "winged-face.hpp"
#include "winged-mesh.hpp"
#include "winged-vertex.hpp"
#include "adjacent-iterator.hpp"
#include "intersection.hpp"

struct ActionCarve::Impl {
  ActionUnit actions;

  void undo () { this->actions.undo (); }
  void redo () { this->actions.redo (); }

  void run (WingedMesh& mesh, const glm::vec3& position, float width) { 
    CarveBrush              brush (width, 0.05f);
    std::unordered_set <Id> ids;
    Sphere                  sphere (position, width);

    mesh.intersectSphere (sphere, ids);

    this->subdivideFaces (sphere, mesh, ids, 0.01f);
    this->carveFaces     (sphere, brush,  mesh, ids);
  }

  void subdivideFaces ( const Sphere& sphere, WingedMesh& mesh
                      , std::unordered_set <Id>& ids, float maxIncircleRadius) {
    std::unordered_set <Id> thisIteration = ids;
    std::unordered_set <Id> nextIteration;

    auto checkNextIteration = [&mesh,&nextIteration,maxIncircleRadius,&sphere] 
      (const WingedFace& face) -> void {
      if (   nextIteration.count (face.id ()) == 0
          && face.incircleRadius (mesh) > maxIncircleRadius
          && IntersectionUtil::intersects (sphere,mesh,face)) {
        nextIteration.insert (face.id ());
      }
    };

    while (thisIteration.size () > 0) {
      for (const Id& id : thisIteration) {
        WingedFace* f = mesh.face (id);
        if (f && f->incircleRadius (mesh) > maxIncircleRadius) {
          std::list <Id> affectedFaces;
          this->actions.add <ActionSubdivide> ()->run (mesh, *f, &affectedFaces);

          for (Id& id : affectedFaces) {
            WingedFace* affected = mesh.face (id);
            if (affected) {
              checkNextIteration (*affected);
              ids.insert (affected->id ());
            }
          }
        }
      }
      thisIteration = nextIteration;
      nextIteration.clear ();
    }
  }

  void carveFaces ( const Sphere& sphere, const CarveBrush& brush
                  , WingedMesh& mesh, const std::unordered_set <Id>& ids) {

    auto carveVertex = [&sphere, &brush, &mesh] (const WingedVertex& vertex) -> glm::vec3 {
      glm::vec3 n    = vertex.normal (mesh);
      glm::vec3 vOld = vertex.vertex (mesh);
      return vOld + (n * brush.y (glm::distance <float> (vOld, sphere.center ())));
    };

    // compute set of vertices
    std::set <WingedVertex*> vertices;
    for (const Id& id : ids) {
      WingedFace* face = mesh.face (id);
      if (face) {
        vertices.insert (&face->firstVertex  ());
        vertices.insert (&face->secondVertex ());
        vertices.insert (&face->thirdVertex  ());
      }
    }

    // compute new positions
    std::list <glm::vec3> newPositions;
    for (WingedVertex* v : vertices) {
      newPositions.push_back (carveVertex (*v));
    }

    // write new positions
    assert (newPositions.size () == vertices.size ());
    auto newPosition = newPositions.begin ();
    for (WingedVertex* v : vertices) {
      this->actions.add <PAModifyVertex> ()->move        (mesh,*v,*newPosition);
      ++newPosition;
    }

    // write normals
    for (WingedVertex* v : vertices) {
      this->actions.add <PAModifyVertex> ()->writeNormal (mesh,*v);
    }
  }
};

DELEGATE_CONSTRUCTOR (ActionCarve)
DELEGATE_DESTRUCTOR  (ActionCarve)
DELEGATE3            (void, ActionCarve, run, WingedMesh&, const glm::vec3&, float)
DELEGATE             (void, ActionCarve, undo)
DELEGATE             (void, ActionCarve, redo)