#pragma once

#include <cassert>
#include <iostream>

namespace stdc {
	
	//This is NOT what you should do, but we do it anyway for now
	//because we are not allowed to crash the server on purpose.
	class bug {};

	namespace detail {
		//uses line to construct error message
		[[nodiscard]] inline auto detailed_error_message(unsigned int line)
			-> std::string
		{
			switch (line % 28) {
				case 0: return "Fixe es jetzt. LG, Sasan";
				case 1: return "Schöne Bescherung!";
				case 2: return "Sieht schlecht aus im Spechthaus.";
				case 3: return "Schade Schokolade!";
				case 4: return "Die Hoffnung stirbt ja bekanntlich zuletzt. Aber sie stirbt.";
				case 5: return "Ach du grüne Neune!";
				case 6: return "Klappe zu, Affe tot!";
				case 7: return "Heute rot, morgen tot.";
				case 8: return "Da können wir jetzt den Brunnen abdecken, nachdem das Kind schon ertrunken ist.";
				case 9: return "Das darf doch wohl nicht wahr sein!";
				case 10: return "Da bleibt einem die Luft weg.";
				case 11: return "Schockschwerenot!";
				case 12: return "Ach du heiliger Bimbam!";
				case 13: return "Da hat der Teufel seine Hand im Spiel.";
				case 14: return "Ich krieg die Motten!";
				case 15: return "Heiligsblechle!";
				case 16: return "Da hat sich jemand einen groben Schnitzer geleistet.";
				case 17: return "Da wurde ein kapitaler Bock geschossen.";
				case 18: return "Mist.";
				case 19: return "Ich sags mal so: Griff ins Klo.";
				case 20: return "Satz mit x: Das war wohl nix.";
				case 21: return "Da müssen wir jetzt die Suppe auslöffeln, die wir uns eingebrockt haben.";
				case 22: return "Da haben wir uns eine Blöße gegeben.";
				case 23: return "Da haben wir die Balken im eigenen Auge nicht gesehen.";
				case 24: return "Das hast du ja wieder schön hingekriegt.";
				case 25: return "Das ist echt ein starkes Stück!";
				case 26: return "Da hat sich wer aber ein richtiges Ei ins eigene Nest gelegt.";
				case 27: return "Jetzt haben wir den Salat.";
				default: return "Da fällt mir nichts mehr ein.";
			}
		}

		[[noreturn]] inline void assert_fail (
			const char *assertion,
			const char *file,
			unsigned int line,
			const char *function
		) {
			auto msg = detailed_error_message(line) + "\n\t\"" + assertion +
				"\"\nfailed in function " + function + " in " + file +
				":" + std::to_string(line) + ".";
			std::clog << msg << '\n';
			assert(false);
			throw bug{}; //NOLINT
		}
	} //namespace detail
} //namespace stdc


//MSVC cannot handle __extension__
#ifndef _MSC_VER
	//alternatives:
	//# define cool_assert(expr) (static_cast<void>(0))
	//# define cool_assert(expr) assert(expr)

	// Suppresses bracked warnings, but at least doesn't warn about useless casts to type bool.
	#define cool_assert(expr) (																				\
		(expr)																								\
			? static_cast<void>(0)																			\
			: stdc::detail::assert_fail(#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__)		\
		)																									\

#else
	#define cool_assert(expr) assert(expr)
#endif
