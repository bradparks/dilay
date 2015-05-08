#ifndef DILAY_OPENGL_BUFFER_ID
#define DILAY_OPENGL_BUFFER_ID

#include "macro.hpp"

class OpenGLBufferId {
  public:
    enum class Type { Buffer, Shader, Program };

    DECLARE_BIG6 (OpenGLBufferId)

    unsigned int id      () const;
    bool         isValid () const;

    void allocate ();
    void reset    ();

  private:
    unsigned int _id;
};

#endif
