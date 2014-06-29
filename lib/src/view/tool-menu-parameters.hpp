#ifndef DILAY_VIEW_TOOL_MENU_PARAMETERS
#define DILAY_VIEW_TOOL_MENU_PARAMETERS

#include <glm/fwd.hpp>
#include "macro.hpp"

class ViewMainWindow;

class ViewToolMenuParameters {
  public:
    DECLARE_BIG6 (ViewToolMenuParameters, ViewMainWindow&, const glm::ivec2&, bool)

    ViewMainWindow&     mainWindow    () const;
    const glm::ivec2&   clickPosition () const;
    bool                rightClick    () const;

  private:
    class Impl;
    Impl* impl;
};

#endif