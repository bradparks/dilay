#include <glm/glm.hpp>
#include <set>
#include "winged-vertex.hpp"
#include "winged-edge.hpp"
#include "winged-face.hpp"
#include "winged-mesh.hpp"
#include "winged-util.hpp"
#include "mesh.hpp"
#include "intersection.hpp"
#include "triangle.hpp"
#include "ray.hpp"
#include "macro.hpp"
#include "adjacent-iterator.hpp"
#include "octree.hpp"
#include "octree-util.hpp"
#include "id.hpp"

struct WingedMesh::Impl {
  const IdObject           id;
  WingedMesh&              wingedMesh;
  Mesh                     mesh;
  std::list <WingedVertex> vertices;
  std::list <WingedEdge>   edges;
  Octree                   octree;
  std::set  <unsigned int> freeFirstIndexNumbers;

  Impl (WingedMesh& p) : wingedMesh (p) {}

  glm::vec3    vertex (unsigned int i) const { return this->mesh.vertex (i); }
  unsigned int index  (unsigned int i) const { return this->mesh.index  (i); }
  glm::vec3    normal (unsigned int i) const { return this->mesh.normal (i); }

  WingedVertex& vertexSLOW (unsigned int i) {
    for (WingedVertex& v : this->vertices) {
      if (v.index () == i)
        return v;
    }
    assert (false);
  }

  WingedVertex& lastVertex () { return this->vertices.back (); }

  WingedEdge& edgeSLOW (const Id& id) {
    for (WingedEdge& e : this->edges) {
      if (e.id () == id)
        return e;
    }
    assert (false);
  }

  WingedFace& face (const Id& id) { return this->octree.faceRef (id); }

  unsigned int addIndex (unsigned int index) { 
    return this->mesh.addIndex (index); 
  }

  WingedVertex& addVertex (const glm::vec3& v, unsigned int l) {
    unsigned int index = this->mesh.addVertex (v);
    this->vertices.emplace_back (index, nullptr, l);
    return this->vertices.back ();
  }

  WingedEdge& addEdge (const WingedEdge& e) {
    this->edges.emplace_back ( e.vertex1 (), e.vertex2 ()
                             , e.leftFace (), e.rightFace ()
                             , e.leftPredecessor (), e.leftSuccessor ()
                             , e.rightPredecessor (), e.rightSuccessor ()
                             , e.previousSibling (), e.nextSibling ()
                             , e.id (), e.isTEdge ());
    this->edges.back ().iterator (--this->edges.end ());
    return this->edges.back ();
  }

  WingedFace& addFace (const WingedFace& f, const Triangle& geometry) {
    unsigned int firstIndexNumber;
    if (this->hasFreeFirstIndexNumber ()) {
      firstIndexNumber = this->nextFreeFirstIndexNumber ();
    }
    else {
      firstIndexNumber = this->mesh.numIndices ();
      this->mesh.allocateIndices (3);
    }
    return this->octree.insertFace 
      ( WingedFace (f.edge (), f.id (), nullptr, firstIndexNumber)
      , geometry);
  }

  void setIndex (unsigned int indexNumber, unsigned int index) { 
    return this->mesh.setIndex (indexNumber, index); 
  }

  void setVertex (unsigned int index, const glm::vec3& v) {
    return this->mesh.setVertex (index,v);
  }

  void setNormal (unsigned int index, const glm::vec3& n) {
    return this->mesh.setNormal (index,n);
  }

  OctreeNode& octreeNodeSLOW (const Id& id) {
    return this->octree.nodeSLOW (id);
  }

  void deleteEdge (const WingedEdge& edge) { this->edges.erase (edge.iterator ()); }
  void deleteFace (const WingedFace& face) { 
    if (face.firstIndexNumber () == this->mesh.numIndices () - 3) {
      this->mesh.popIndices (3);
    }
    else {
      this->freeFirstIndexNumbers.insert (face.firstIndexNumber ());
    }
    this->octree.deleteFace (face); 
  }

  void popVertex () {
    this->mesh.popVertex ();
    this->vertices.pop_back ();
  }

  WingedFace& realignFace (const WingedFace& face, const Triangle& triangle) {
    return this->octree.realignFace (face, triangle);
  }

  unsigned int numVertices       () const { return this->mesh.numVertices (); }
  unsigned int numWingedVertices () const { return this->vertices.size    (); }
  unsigned int numEdges          () const { return this->edges.size       (); }
  unsigned int numFaces          () const { 
    return OctreeUtil :: numFaces (this->octree); 
  }
  unsigned int numIndices        () const { return this->mesh.numIndices  (); }

  void write () {
    // Indices
    if (this->freeFirstIndexNumbers.size () > 0) {
      unsigned int fin = 0;
      for (OctreeFaceIterator it = this->octree.faceIterator (); it.isValid (); it.next ()) {
        it.element ().writeIndices (this->wingedMesh, &fin);
        fin = fin + 3;
      }
      this->mesh.resizeIndices (this->numFaces () * 3);
      this->freeFirstIndexNumbers.clear ();
    }
    else {
      for (OctreeFaceIterator it = this->octree.faceIterator (); it.isValid (); it.next ()) {
        it.element ().writeIndices (this->wingedMesh);
      }
    }
    // Normals
    for (WingedVertex& v : this->vertices) {
      v.writeNormal (this->wingedMesh);
    }
  }

  void bufferData  () { 
    assert (this->freeFirstIndexNumbers.size () == 0);
    this->mesh.bufferData   (); 
  }

  void render      () { 
    this->mesh.render   (); 
#ifdef DILAY_RENDER_OCTREE
    this->octree.render ();
#endif
  }

  void reset () {
    this->mesh    .reset ();
    this->vertices.clear ();
    this->edges   .clear ();
    this->octree  .reset ();
  }

  void reset (const glm::vec3& center, float width) {
    this->reset ();
    this->octree.reset (center,width);
  }

  void toggleRenderMode () { this->mesh.toggleRenderMode (); }

  void intersectRay (const Ray& ray, FaceIntersection& intersection) {
    this->octree.intersectRay (this->wingedMesh,ray,intersection);
  }

  void intersectSphere (const Sphere& sphere, std::list <Id>& ids) {
    this->octree.intersectSphere (this->wingedMesh,sphere,ids);
  }

  void intersectSphere (const Sphere& sphere, std::unordered_set <WingedVertex*>& vertices) {
    this->octree.intersectSphere (this->wingedMesh,sphere,vertices);
  }

  bool hasFreeFirstIndexNumber () const { 
    return this->freeFirstIndexNumbers.size () > 0;
  }
  
  unsigned int nextFreeFirstIndexNumber () {
    assert (this->hasFreeFirstIndexNumber ());
    unsigned int indexNumber = *this->freeFirstIndexNumbers.begin ();
    this->freeFirstIndexNumbers.erase (this->freeFirstIndexNumbers.begin ());
    return indexNumber;
  }
};

WingedMesh :: WingedMesh () : impl (new WingedMesh::Impl (*this)) {}

WingedMesh :: WingedMesh (const WingedMesh& source) 
  : impl (new WingedMesh::Impl (*this)) {
    WingedUtil :: fromMesh (*this, source.impl->mesh);
}

const WingedMesh& WingedMesh :: operator= (const WingedMesh& source) {
  if (this == &source) return *this;
  WingedUtil :: fromMesh (*this, source.impl->mesh);
  return *this;
}
DELEGATE_DESTRUCTOR (WingedMesh)
ID                  (WingedMesh)

DELEGATE1_CONST (glm::vec3      , WingedMesh, vertex, unsigned int)
DELEGATE1_CONST (unsigned int   , WingedMesh, index, unsigned int)
DELEGATE1_CONST (glm::vec3      , WingedMesh, normal, unsigned int)
DELEGATE1       (WingedVertex&  , WingedMesh, vertexSLOW, unsigned int)
DELEGATE        (WingedVertex&  , WingedMesh, lastVertex)
DELEGATE1       (WingedEdge&    , WingedMesh, edgeSLOW, const Id&)
DELEGATE1       (WingedFace&    , WingedMesh, face, const Id&)

DELEGATE1       (unsigned int   , WingedMesh, addIndex, unsigned int)
DELEGATE2       (WingedVertex&  , WingedMesh, addVertex, const glm::vec3&, unsigned int)
DELEGATE1       (WingedEdge&    , WingedMesh, addEdge, const WingedEdge&)
DELEGATE2       (WingedFace&    , WingedMesh, addFace, const WingedFace&, const Triangle&)
DELEGATE2       (void           , WingedMesh, setIndex, unsigned int, unsigned int)
DELEGATE2       (void           , WingedMesh, setVertex, unsigned int, const glm::vec3&)
DELEGATE2       (void           , WingedMesh, setNormal, unsigned int, const glm::vec3&)

GETTER_CONST    (const Vertices&, WingedMesh, vertices)
GETTER_CONST    (const Edges&   , WingedMesh, edges)
GETTER_CONST    (const Octree&  , WingedMesh, octree)
DELEGATE1       (OctreeNode&    , WingedMesh, octreeNodeSLOW, const Id&)

DELEGATE1       (void        , WingedMesh, deleteEdge, const WingedEdge&)
DELEGATE1       (void        , WingedMesh, deleteFace, const WingedFace&)
DELEGATE        (void        , WingedMesh, popVertex)
DELEGATE2       (WingedFace& , WingedMesh, realignFace, const WingedFace&, const Triangle&)
 
DELEGATE_CONST  (unsigned int, WingedMesh, numVertices)
DELEGATE_CONST  (unsigned int, WingedMesh, numWingedVertices)
DELEGATE_CONST  (unsigned int, WingedMesh, numEdges)
DELEGATE_CONST  (unsigned int, WingedMesh, numFaces)
DELEGATE_CONST  (unsigned int, WingedMesh, numIndices)

DELEGATE        (void, WingedMesh, write)
DELEGATE        (void, WingedMesh, bufferData)
DELEGATE        (void, WingedMesh, render)
DELEGATE        (void, WingedMesh, reset)
DELEGATE2       (void, WingedMesh, reset, const glm::vec3&, float)
DELEGATE        (void, WingedMesh, toggleRenderMode)

DELEGATE2       (void, WingedMesh, intersectRay, const Ray&, FaceIntersection&)
DELEGATE2       (void, WingedMesh, intersectSphere, const Sphere&, std::list<Id>&)
DELEGATE2       (void, WingedMesh, intersectSphere, const Sphere&, std::unordered_set<WingedVertex*>&)
