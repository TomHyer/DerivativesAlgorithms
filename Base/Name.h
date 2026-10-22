
// Identifier whose too-similar spellings are treated as ambiguous
// Identity, ordering, and hash use the canonical (condensed uppercase) form
// Source() is the original spelling, for display and AmbiguousWith()

#pragma once

#include <compare>
#include <functional>
#include <string>
#include "Export.h"

class BASE_EXPORT Name_
{
	std::string source_;
	std::string canonical_;
public:
	Name_() = default;
	Name_(std::string source);
	Name_(const char* source);

	const std::string& Source() const { return source_; }
	const std::string& Canonical() const { return canonical_; }
	bool Empty() const { return canonical_.empty(); }

	// same canonical form, different spelling
	bool AmbiguousWith(const Name_& other) const
	{
		return canonical_ == other.canonical_ && source_ != other.source_;
	}

	bool operator==(const Name_& rhs) const { return canonical_ == rhs.canonical_; }
	std::strong_ordering operator<=>(const Name_& rhs) const { return canonical_ <=> rhs.canonical_; }
};

template<> struct std::hash<Name_>
{
	std::size_t operator()(const Name_& name) const noexcept
	{
		return std::hash<std::string>{}(name.Canonical());
	}
};
