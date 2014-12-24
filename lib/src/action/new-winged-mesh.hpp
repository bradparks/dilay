#ifndef DILAY_ACTION_NEW_WINGED_MESH
#define DILAY_ACTION_NEW_WINGED_MESH

#include "action.hpp"
#include "macro.hpp"

class MeshDefinition;
class Scene;
class WingedMesh;

class ActionNewWingedMesh : public Action {
  public: 
    DECLARE_BIG3 (ActionNewWingedMesh)

    WingedMesh& run (Scene&, const MeshDefinition&);

  private:
    void runUndo () const;
    void runRedo () const;

    IMPLEMENTATION
};

#endif
