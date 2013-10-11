#ifndef DILAY_ID
#define DILAY_ID

#include <iosfwd>

typedef unsigned int IdPrimitive;

// Valid `Id`s can only be generated by `IDObject`s
class Id {
  public: Id (const Id& id) : _id (id .get ()) {}
          Id ()             : _id (0)          {}

    friend class IdObject;

    bool operator== (const Id& id) const { return this->_id == id.get (); }
    IdPrimitive get ()             const { return this->_id; }
    bool isInvalid  ()             const { return this->_id == 0; }
    bool isValid    ()             const { return ! this->isInvalid (); }

    Id (IdPrimitive idp) : _id (idp) {} // make this private again
  private:
    const IdPrimitive _id;
};

std::ostream& operator<<(std::ostream&, const Id&);

// Generates `Id`s
class IdObject {
  public:    IdObject (const Id& = IdObject::invalidId ());
    explicit IdObject (const IdObject&);

    const IdObject& operator= (const IdObject&) {
      return *this;
    }
           Id id        () const { return Id (this->_id); }
    static Id invalidId ()       { return Id (); }
    static Id staticId  ()       { return Id (IdObject::nextId ()); }

  private:
    const  IdPrimitive _id;
    static IdPrimitive nextId ();
};

#endif
