#ifndef SORT_H_
#define SORT_H_

#include <cstddef>

class card;
class effect;

struct card_sort {
	bool operator()(card* const& c1, card* const& c2) const;
};
// Bot Theater: containers of cards and effects are hashed and ordered by creation number instead
// of memory address, so the same duel (same seed, same choices) always plays out the same way.
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
