#include <unordered_set>
#include <functional>
#include <glm/glm.hpp>
#include "action/subdivide.hpp"
#include "macro.hpp"
#include "winged-face.hpp"
#include "winged-edge.hpp"
#include "winged-vertex.hpp"
#include "action/unit.hpp"
#include "partial-action/triangulate-6-gon.hpp"
#include "partial-action/triangulate-quad.hpp"
#include "partial-action/delete-t-edges.hpp"
#include "partial-action/insert-edge-vertex.hpp"
#include "adjacent-iterator.hpp"
#include "subdivision-butterfly.hpp"

typedef std::unordered_set <WingedFace*> FaceSet;

struct ActionSubdivide::Impl {
  ActionUnit actions;

  void undo () { this->actions.undo (); }
  void redo () { this->actions.redo (); }

  void run (WingedMesh& mesh, WingedFace& face) { 
    this->actions.reset ();
    this->subdivide (mesh, face.level (), face); 
  }

  void subdivide (WingedMesh& mesh, unsigned int selectionLevel, WingedFace& selection) {
    if (selection.level () <= selectionLevel) {
      FaceSet neighbourhood, border;

      if (! this->oneRingNeighbourhood (mesh, selection, selectionLevel, neighbourhood)) {
        return this->subdivide (mesh, selectionLevel, selection);
      }
      
      if (! this->oneRingBorder (mesh, selectionLevel, neighbourhood, border)) {
        return this->subdivide (mesh, selectionLevel, selection);
      }

      this->subdivideFaces (mesh, neighbourhood, selectionLevel);
      this->refineBorder   (mesh, border, selectionLevel);
    }
  }

  bool oneRingNeighbourhood ( WingedMesh& mesh, WingedFace& selection
                            , unsigned int selectionLevel, FaceSet& neighbourhood) {
    neighbourhood.clear  ();
    neighbourhood.insert (&selection);

    for (auto vIt = selection.adjacentVertexIterator (true); vIt.isValid (); vIt.next ()) {
      for (ADJACENT_FACE_ITERATOR (fIt, vIt.element ())) {
        WingedFace& face = fIt.element ();
        unsigned int faceLevel = face.level ();
        if (faceLevel < selectionLevel) {
          this->subdivide (mesh, faceLevel, face);
          return false;
        }
        else if (faceLevel == selectionLevel) { 
          this->insertNeighbour (neighbourhood, face);
        }
      }
    }
    return true;
  }

  bool oneRingBorder ( WingedMesh& mesh, unsigned int selectionLevel
                     , FaceSet& neighbourhood, FaceSet& border) {
    FaceSet extendedNeighbourhood;

    // checks whether a face is already a neighbour
    std::function < bool (WingedFace&) > isNeighbour =
      [&neighbourhood,&extendedNeighbourhood] (WingedFace& face) {

        return (neighbourhood        .count (&face) > 0) 
            || (extendedNeighbourhood.count (&face) > 0);
      };

    // checks whether a face has two adjacent faces that are neighbours
    std::function < bool (WingedFace&) > hasAtLeast2Neighbours =
      [&isNeighbour] (WingedFace& face) {
        unsigned int numNeighbours = 0;

        for (ADJACENT_FACE_ITERATOR (it, face)) {
          if (isNeighbour (it.element ())) {
            numNeighbours++;
          }
        }
        return numNeighbours >= 2;
      };

    std::function < bool (WingedFace&) > makeBorder;

    // checks whether a neighbourhood-adjacent face needs to be subdivided OR whether it is 
    // actually a neighbour-face itself OR whether it is a border-face
    std::function < bool (WingedFace&) > checkAdjacent = [ & ] (WingedFace& face) {
      if (! isNeighbour (face)) {
        unsigned int faceLevel = face.level ();

        if (faceLevel < selectionLevel) {
          this->subdivide (mesh,faceLevel,face);
          return false;
        }
        else if (faceLevel == selectionLevel) {
          if (face.tEdge ()) {
            this->insertNeighbour (extendedNeighbourhood,face);
            border.erase      (&face);
            return makeBorder (face);
          }
          else {
            if (hasAtLeast2Neighbours (face)) {
              this->insertNeighbour (extendedNeighbourhood,face);
              border.erase      (&face);
              return makeBorder (face);
            }
            else {
              border.insert (&face);
              return true;
            }
          }
        }
      }
      return true;
    };

    // builds the border by traversing a neighbour's adjacent faces
    makeBorder = [ & ] (WingedFace& neighbour) {
      for (auto it = neighbour.adjacentFaceIterator (true); it.isValid (); it.next ()) {
        if (! checkAdjacent (it.element ()))
          return false;
      }
      return true;
    };

    border.clear ();

    for (WingedFace* neighbour : neighbourhood) {
      if (! makeBorder (*neighbour)) {
        return false;
      }
    }
    neighbourhood.insert (extendedNeighbourhood.begin (), extendedNeighbourhood.end ());
    return true;
  }

  void insertNeighbour (FaceSet& neighbourhood, WingedFace& neighbour) {
    neighbourhood.insert (&neighbour);

    WingedEdge* tEdge = neighbour.tEdge ();
    if (tEdge) {
      neighbourhood.insert (tEdge->otherFace (neighbour));
    }
  }


  void subdivideFaces (WingedMesh& mesh, FaceSet& faces, unsigned int selectionLevel) {
    this->actions.add <PADeleteTEdges> ()->run (mesh,faces);

    for (WingedFace* face : faces) {
      for (auto it = face->adjacentEdgeIterator (); it.isValid (); ) {
        WingedEdge& edge = it.element ();
        assert (! edge.isTEdge ());
        it.next ();

        if (   edge.vertex1Ref ().level () <= selectionLevel
            && edge.vertex2Ref ().level () <= selectionLevel) {

          this->actions.add <PAInsertEdgeVertex> ()->run 
            (mesh, edge, SubdivisionButterfly::subdivideEdge (mesh, selectionLevel, edge));
        }
      }
    }
    for (WingedFace* face : faces) {
      this->actions.add <PATriangulate6Gon> ()->run (mesh,*face);
    }
  }

  void refineBorder (WingedMesh& mesh, FaceSet& border, unsigned int selectionLevel) {
    this->actions.add <PADeleteTEdges> ()->run (mesh, border);

    for (WingedFace* face : border) {
      assert (face->level () == selectionLevel);
      assert (face->tEdge () == nullptr);
      this->actions.add <PATriangulateQuad> ()->run (mesh, *face);
    }
  }
};

DELEGATE_CONSTRUCTOR (ActionSubdivide)
DELEGATE_DESTRUCTOR  (ActionSubdivide)
DELEGATE2            (void, ActionSubdivide, run, WingedMesh&, WingedFace&)
DELEGATE             (void, ActionSubdivide, undo)
DELEGATE             (void, ActionSubdivide, redo)
