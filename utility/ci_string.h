#ifndef CI_STRING_H
#define CI_STRING_H

#include <string>

// From Herb Sutter's GotW #29: case-insensitive strings
// KBH modified: templated on character type

template <typename Char>
struct ci_char_traits : public std::char_traits<Char> {
// just inherit all the other functions that we don't need to override
	static bool eq(Char c1, Char c2)
		{ return toupper(c1) == toupper(c2); }

	static bool lt(Char c1, Char c2)
		{ return toupper(c1) <  toupper(c2); }

	static int compare(const Char *s1, const Char *s2, size_t count) {
		for (;count--; ++s1, ++s2)
			if (!eq(*s1, *s2))
				return (lt(*s1, *s2) ? -1 : +1);
		return 0;
	}

	static const Char* find(const Char* s, int n, Char a) {
		while(n-- > 0 && toupper(*s) != toupper(a))
			++s;
		return s;
	}
};

template <typename C>
using ci_basic_string = std::basic_string<C, ci_char_traits<C>>;

using ci_string = ci_basic_string<char>;
#endif

#ifndef CI_STRING_NO_OSTREAM
#define CI_STRING_NO_OSTREAM
#include <ostream>
template <typename C>
std::ostream& operator<<(std::ostream& os, const ci_basic_string<C>s)
	{ return os << s.c_str(); }
#endif