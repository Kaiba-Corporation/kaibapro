#ifndef SORT_H_
#define SORT_H_

#include <cstddef>

class card;
class effect;

struct card_sort {
	bool operator()(card* const& c1, card* const& c2) const;
};
// Four-player work: containers of cards and effects are hashed and ordered by creation number
// instead of memory address, so a duel plays out the same way every time it is replayed.
struct card_hash {
	std::size_t operator()(card* const& c) const;
};
struct effect_hash {
	std::size_t operator()(effect* const& e) const;
};
struct effect_sort {
	bool operator()(effect* const& e1, effect* const& e2) const;
};

#endif /* SORT_H_ */
