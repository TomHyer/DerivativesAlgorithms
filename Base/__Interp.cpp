
#include "__Platform.h"
#include "Strict.h"

#include "Interp.h"

namespace
{
/*IF--------------------------------------------------------------------------
public Interp1_New_Linear
   Create a linear interpolator
&inputs
name is string
   A name for the object being created
x is number[]
   &IsMonotonic(x)\$ values must be in ascending order
   The x-values (abcissas)
y is number[]
   &$.size() == x.size()\x and y must have the same size
   The values of f(x) at each x
&outputs
f is handle Interp1
   The interpolator
-IF-------------------------------------------------------------------------*/

   void Interp1_New_Linear
      (const String_& name,
       const Vector_<>& x,
       const Vector_<>& y,
       Handle_<Interp1_>* f)
   {
      f->reset(new Interp1Linear_(name, x, y));
   }

/*IF--------------------------------------------------------------------------
public Interp1_Get
   Interpolate a value at specified abcissas
&inputs
f is handle Interp1
   The interpolant function
x is number[]
   The x-values (abcissas)
&outputs
y is number[]
   The interpolated function values at x-values
-IF-------------------------------------------------------------------------*/

   double CheckedInterp(const Interp1_& f, double x)
   {
      REQUIRE(f.IsInBounds(x), "X (= " + std::to_string(x) + ") is outside interpolation domain");
      return f(x);
   }

   void Interp1_Get
      (const Handle_<Interp1_>& f,
      const Vector_<>& x,
      Vector_<>* y)
   {
      *y = Apply([&](double x_i){return CheckedInterp(*f, x_i); }, x);
   }

}   // leave local

#include "MG_Interp1_New_Linear_public.inc"
#include "MG_Interp1_Get_public.inc"


#include "_Cookie.h"


extern "C" __declspec(dllexport) double ffi_Interp1_New_Linear
   (const char* f_name, int f_x, int f_y)
{
   Cookie::Output_ retval;
   const char* argName = 0;
   try
   {
      Cookie::Input_ get;
      Log::Write("Interp1_New_Linear");
      argName = "name (input #1)";
      const String_ name(f_name);
      argName = "x (input #2)";
      const Vector_<double> x = get.VectorDouble(f_x);
      argName = "y (input #3)";
      const Vector_<double> y = get.VectorDouble(f_y);
      argName = 0;

      Handle_<Interp1_> f;
      Interp1_New_Linear(name, x, y, &f);
      retval.Append(f);
      return retval.Finalize();
   }
   catch (std::exception& e)
   {
      return retval.Error(e.what(), argName);
   }
   catch (...)
   {
      return retval.Error("Unknown error", argName);
   }
}

extern "C" __declspec(dllexport) double ffi_Interp1_Get
   (int f_f, int f_x)
{
   Cookie::Output_ retval;
   const char* argName = 0;
   try
   {
      Cookie::Input_ get;
      Log::Write("Interp1_Get");
      argName = "f (input #1)";
      const Handle_<Interp1_> f = get.Handle<Interp1_>(f_f);
      argName = "x (input #2)";
      const Vector_<double> x = get.VectorDouble(f_x);
      argName = 0;

      Vector_<double> y;
      Interp1_Get(f, x, &y);
      retval.Append(y);
      return retval.Finalize();
   }
   catch (std::exception& e)
   {
      return retval.Error(e.what(), argName);
   }
   catch (...)
   {
      return retval.Error("Unknown error", argName);
   }
}


