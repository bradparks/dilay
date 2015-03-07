#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <glm/glm.hpp>
#include "cache.hpp"
#include "sculpt-brush/carve.hpp"
#include "state.hpp"
#include "tool/sculpt/behaviors.hpp"
#include "tool/util/movement.hpp"
#include "view/cursor/inner-radius.hpp"
#include "view/properties.hpp"
#include "view/tool/tip.hpp"
#include "view/util.hpp"

struct ToolSculptGrab::Impl {
  ToolSculptGrab*       self;
  SculptBrushCarve      brush;
  ToolUtilMovement      movement;
  ViewCursorInnerRadius cursor;

  Impl (ToolSculptGrab* s) 
    : self (s)
    , movement ( this->self->state ().camera ()
               , glm::vec3 (0.0f)
               , MovementConstraint::Explicit )
  {}

  void runSetupBrush () {
    this->brush.innerRadiusFactor (this->self->cache ().get <float> ("inner-radius-factor", 0.5f));
    this->brush.useLastPosition   (true);
    this->brush.useIntersection   (true);
  }

  void runSetupCursor () {
    this->cursor.innerRadiusFactor (this->brush.innerRadiusFactor ());
  }

  void runSetupProperties (ViewPropertiesPart& properties) {
    QDoubleSpinBox& innerRadiusEdit = ViewUtil::spinBox ( 0.0f, this->brush.innerRadiusFactor ()
                                                        , 1.0f, 0.1f );
    ViewUtil::connect (innerRadiusEdit, [this] (float f) {
      this->brush.innerRadiusFactor  (f);
      this->cursor.innerRadiusFactor (f);
      this->self->cache ().set ("inner-radius-factor", f);
    });
    properties.add (QObject::tr ("Inner radius"), innerRadiusEdit);
  }

  void runSetupToolTip (ViewToolTip& toolTip) {
    toolTip.add (ViewToolTip::MouseEvent::Left, QObject::tr ("Drag to sculpt"));
  }

  void runMouseMoveEvent (const QMouseEvent& e) {
    if (e.buttons () == Qt::NoButton) {
      this->self->updateCursorByIntersection (e);
    }
    else if (e.buttons () == Qt::LeftButton && this->brush.hasPosition ()) {
      const glm::vec3 oldBrushPos = this->brush.position ();

      if ( this->movement.move (ViewUtil::toIVec2 (e))
        && this->brush.updatePosition (this->movement.position ()) )
      {
        this->brush.direction       (this->brush.position () - oldBrushPos);
        this->brush.intensityFactor (1.0f / this->brush.radius ());
        this->self->sculpt ();
      }
    }
  }

  void runMousePressEvent (const QMouseEvent& e) {
    this->self->initializeDragMovement (this->movement, e);
  }
};

DELEGATE_TOOL_SCULPT_BEHAVIOR (ToolSculptGrab)
