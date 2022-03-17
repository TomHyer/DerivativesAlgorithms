
// interface readers to support cross-language translation of inputs

#pragma once

#include "Exceptions.h"
#include "Environment.h"

class Date_;
class Storable_;

class BASE_EXPORT UIRow_ : noncopyable
{
public:
   virtual double ExtractDouble(int i_col) const = 0;
   virtual double ExtractDouble(int i_col, double def_val) const = 0;
   virtual int ExtractInt(int i_col) const = 0;
   virtual int ExtractInt(int i_col, int def_val) const = 0;
   virtual bool ExtractBool(int i_col) const = 0;
   virtual String_ ExtractString(int i_col) const = 0;
   virtual Date_ ExtractDate(int i_col) const = 0;
   virtual Handle_<Storable_> ExtractHandleBase(_ENV, int i_col) const = 0;

   template<class T_> T_ ExtractEnum(int i_col) const { return T_(ExtractString(i_col)); }
   template<class T_> Handle_<T_> ExtractHandle(_ENV, int i_col, bool optional = false) const 
   {
      auto base = ExtractHandleBase(_env, i_col);
      if (!base)
      {
         REQUIRE(optional, "Missing handle in record");
         return Handle_<T_>();
      }
      auto retval = handle_cast<T_>(base);
      REQUIRE(retval, "Handle has wrong type in record");
      return retval;
   }
};

