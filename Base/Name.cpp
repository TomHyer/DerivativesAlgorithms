
#include "Platform.h"
#include "Name.h"
#include <cctype>
#include "Strict.h"

namespace
{
	bool IsFluff(unsigned char c)
	{
		return c == ' ' || c == '\t' || c == '_';
	}

	std::string CanonicalForm(const std::string& src)
	{
		std::string retval;
		retval.reserve(src.size());
		for (unsigned char c : src)
		{
			if (!IsFluff(c))
				retval.push_back(static_cast<char>(std::toupper(c)));
		}
		return retval;
	}
}

Name_::Name_(std::string source)
	:
source_(std::move(source)),
canonical_(CanonicalForm(source_))
{	}

Name_::Name_(const char* source)
	:
Name_(std::string(source ? source : ""))
{	}
