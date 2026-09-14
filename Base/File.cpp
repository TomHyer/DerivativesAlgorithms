
#include "Platform.h"
#include "File.h"
#include <fstream>
#include "Strict.h"

#include "Vectors.h"
#include "Exceptions.h"

void File::Read
	(const String_& filename,
	 Vector_<String_>* dst)
{
	std::ifstream src(filename.c_str());  // fstream has no ci_traits overload
	REQUIRE(src, "Can't open '" + filename + "'");
	for (std::string line; std::getline(src, line); )
		dst->emplace_back(line.begin(), line.end());
}

