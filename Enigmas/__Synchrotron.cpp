
#include "Base/__Platform.h"
#include <map>
#include <regex>
#include <array>
#include "Base/Strict.h"

#include "Base/Archive.h"
#include "Base/Optionals.h"
#include "Base/Numerics.h"

/*IF--------------------------------------------------------------------------
storable Lexicon
    List of "valid" words
&members
name is ?string
words is string[]
    Words considered valid
-IF-------------------------------------------------------------------------*/

namespace
{
#include "MG_Lexicon_Write.inc"

    struct Lexicon_ : Storable_
    {
		Vector_<String_> words_;
		Lexicon_(const String_& name, const Vector_<String_>& words) : Storable_("Lexicon", name), words_(words)
		{
			auto less = [](const String_& lhs, const String_& rhs)
				{
					return lhs.size() == rhs.size()
						? lhs < rhs
						: lhs.size() < rhs.size();
				};
			Sort(&words_, less);
		}
		void Write(Archive::Store_& dst) const override
		{
			Lexicon::XWrite(dst, name_, words_);
		}
    };

#include "MG_Lexicon_Read.inc"


/*IF--------------------------------------------------------------------------
public Lexicon_New
   Create a storable object containing a block of data
&inputs
words is string[]
   Valid words
&optional
name is string
   A name for the object being created
&outputs
object is handle Lexicon
   The new object
-IF-------------------------------------------------------------------------*/

   void Lexicon_New
      (const Vector_<String_>& words,
       const String_& name,
       Handle_<Lexicon_>* object)
   {
      object->reset(new Lexicon_(name, words));
   }


   struct Topology_
    {
        Vector_<Vector_<int>> atNode_;      // indices of attached links
        Vector_<pair<int, int>> atLink_;    // nodes connected by the link
    };

    Topology_ MakeTopology(int n_nodes, const Matrix_<Cell_>& links_in, int link_col_1 = 1, int link_col_2 = 2)
    {
        Topology_ retval;
        retval.atNode_.Resize(n_nodes);
        for (int ir = 0; ir < links_in.Rows(); ++ir)
        {
            int n1 = Cell::ToInt(links_in(ir, link_col_1)), n2 = Cell::ToInt(links_in(ir, link_col_2));
            REQUIRE(Min(n1, n2) >= 0 && Max(n1, n2) < n_nodes, "Node indices out of range");
            retval.atNode_[n1].push_back(retval.atLink_.size());
            retval.atNode_[n2].push_back(retval.atLink_.size());
            retval.atLink_.emplace_back(n1, n2);
        }
        return retval;
    }

    struct Game_
    {
        Topology_ top_;
        String_ nodes_;     // letter at each node
        String_ links_;     // letter at each link

        Game_(const Topology_& t) : top_(t), nodes_(t.atNode_.size(), ' '), links_(t.atLink_.size(), ' ')
        {   }
    };


    // specialized dictionary for Ghost-like games
    struct Dict_
    {
        Vector_<String_> words_;
        using iter_t = Vector_<String_>::const_iterator;

        struct Range_
        {
            iter_t begin_, end_;
            int loc_;   // which character to examine

            Range_(iter_t begin, iter_t end, int loc = 0) : begin_(begin), end_(end), loc_(loc) {}

            iter_t begin() { return begin_; }
            iter_t end() { return end_; }
            bool empty() { return end_ == begin_; }
            void clear() { end_ = begin_; }

            Range_ Find(char c) const
            {
                auto start = begin_, stop = end_;
                if (begin_ != end_ && begin_->size() > loc_)
                {
                    if (c != 'a')
                        start = partition_point(start, stop, [=](const String_& s) { return s[loc_] < c; });
                    if (c != 'z')
                        stop = partition_point(start, stop, [=](const String_& s) { return s[loc_] <= c; });
                }
                return Range_(start, stop, loc_ + 1);
            }
        };

        Dict_(Vector_<String_>&& words) : words_(words)
        {
            auto less = [](const String_& lhs, const String_& rhs)
                {
                    return lhs.size() == rhs.size()
                        ? lhs < rhs
                        : lhs.size() < rhs.size();
                };
            Sort(&words_, less);
        }

        int MaxLength() const
        {
            return words_.empty() ? 0 : int(words_.back().size());
        }

        auto Shorter(int len) const
        {
            return [=](const String_& s) { return s.size() < len; };
        }

        Range_ OfLength(int len) const
        {
            iter_t start = partition_point(words_.begin(), words_.end(), Shorter(len)),
                stop = partition_point(start, words_.end(), Shorter(len + 1));
            return Range_(start, stop, 0);
        }
    };

    struct Solver_
    {
        struct State_
        {
            char own_;
            Vector_<pair<char, const State_*>> links_;
        };

        Game_ game_;
        Vector_<State_> states_;

        Solver_(const Game_& game) : game_(game), states_(game.nodes_.size())
        {
            for (int ii = 0; ii < states_.size(); ++ii)
            {
                State_& si = states_[ii];
                si.own_ = game.nodes_[ii];
                if (si.own_ == ' ')
                    continue;   // end of the road
                const auto& linkLocs = game.top_.atNode_[ii];
                for (int jj = 0; jj < linkLocs.size(); ++jj)
                {
                    int iLink = linkLocs[jj];
                    char cLink = game_.links_[iLink];
                    if (cLink == ' ')
                        continue;
                    auto aLink = game_.top_.atLink_[iLink];
                    int other = aLink.first + aLink.second - ii;
                    for (const auto& pvs : si.links_)
                        REQUIRE(pvs.first != cLink, "Duplicated link character '" + String_(1, cLink) + "'");
                    si.links_.emplace_back(cLink, &states_[other]);
                }
            }
        }

        Vector_<pair<String_, String_>> FindAll(const Dict_& words, int min_length, const std::optional<int>& start_node) const
        {
            Vector_<pair<String_, String_>> retval;
            char startLetter = start_node.has_value() ? game_.nodes_[start_node.value()] : 0;
            startLetter = startLetter >= 'a' && startLetter <= 'z' ? startLetter : 0;
            for (int nLen = min_length; nLen <= words.MaxLength(); ++nLen)
            {
                auto nRange = words.OfLength(nLen), lRange0 = words.OfLength(nLen - (startLetter ? 1 : 0));
                // outer loop is first node CHOICE (not necessarily first node)
                if (startLetter)
                    nRange = nRange.Find(startLetter);
                for (const State_& s : states_)
                {
                    auto lRange = lRange0;
                    if (start_node.has_value() && (&states_[start_node.value()] != &s))
                        continue;  // this is not our starting point
                    auto nRange1 = (start_node.has_value() && !startLetter) ? nRange : nRange.Find(s.own_);
                    if (nRange1.empty())
                        continue;
                    for (const String_& ls : lRange)
                    {
                        auto nLive = nRange1;
                        const State_* sLive = &s;
                        for (auto lc : ls)  // walk forward through the link-word
                        {
                            // where does this link-char take us? must be unique
                            auto isMe = [=](const pair<char, const State_*>& val) { return val.first == lc; };
                            auto pl = find_if(sLive->links_.begin(), sLive->links_.end(), isMe);
                            if (pl == sLive->links_.end())
                            {
                                nLive.clear();
                                break;  // the letter we need isn't here
                            }
                            sLive = pl->second;  // other end of link
                            nLive = nLive.Find(sLive->own_);
                            if (nLive.empty())
                                break;  // this node doesn't lead to any words
                        }
                        if (!nLive.empty())
                        {
                            assert(nLive.end() == Next(nLive.begin()));
                            retval.emplace_back(*nLive.begin(), ls);
                        }
                    }
                }
            }
            return retval;
        }
    };

/*IF--------------------------------------------------------------------------
public Synchrotron_Solutions
    Find all the solutions (subject to minimum length)
&inputs
nodes is string
    The node characters
links is cell[][]
    &$.Cols() == 3\$ must have three columns (char, two ends)
    Links and corresponding characters
dict is handle Lexicon
    All the words
&optional
min_length is integer (3)
    Minimum length of longer word
start_node is integer
    If supplied, start node is fixed and the two words are of the same length
&outputs
solutions is string[][]
    One row per solution
-IF-------------------------------------------------------------------------*/

    void Synchrotron_Solutions
        (const String_& nodes,
         const Matrix_<Cell_>& links,
         const Handle_<Lexicon_>& dict,
         int min_length,
         const std::optional<int>& start_node,
         Matrix_<String_>* solutions)
    {
        auto top = MakeTopology(int(nodes.size()), links);
        Game_ game(top);
        game.nodes_ = nodes;
        for (int ir = 0; ir < links.Rows(); ++ir)
        {
            String_ temp = Cell::ToString(links(ir, 0));
            game.links_[ir] = temp.empty() ? ' ' : temp[0];
        }
        Vector_<pair<String_, String_>> retval = Solver_(game).FindAll(Vector_<String_>(dict->words_), min_length, start_node);
        solutions->Resize(retval.size(), retval.empty() ? 0 : 2);
        for (int is = 0, id = retval.size() - 1; is < retval.size(); ++is, --id)
        {
            (*solutions)(id, 0) = retval[is].first;
            (*solutions)(id, 1) = retval[is].second;
        }
    }


/*IF--------------------------------------------------------------------------
public Synchrotron_MatchWords
    Find words by equivalence up to letter-substitution
&inputs
target is string
    The word or string to match
dict is handle Lexicon
    All the words
&optional
sort is boolean (true)
	If true, output is sorted to display words with less common letters first
&outputs
solutions is string[]
    Words which can be transformed to the target via reversible letter substitution
-IF-------------------------------------------------------------------------*/

    void SortRarest(Vector_<String_>* words)
    {
        std::map<char, int> freq;
        for (const auto& w : *words)
            for (const auto& c : w)
                ++freq[c];

        auto score = [&](const String_& s)
            {
                int retval = 0;
                for (const auto& c : s)
                    retval += freq[c];
                return retval;
            };

        auto less = [&](const String_& lhs, const String_& rhs)
            {
                return score(lhs) < score(rhs);
            };
        Sort(words, less);
    }

    void Synchrotron_MatchWords
        (const String_& target,
         const Handle_<Lexicon_>& dict,
		 bool sort,
         Vector_<String_>* solutions)
    {
        // this would be faster as a vector comparison with short circuit; but this way enables whole-dict precomputation
        auto toKey = [](const String_& s) {
            String_ retval = s;
            for (int i = 0; i < s.size(); ++i)
                retval[i] = 'a' + static_cast<char>(s.find(s[i]));
            return retval;
        };
        String_ match = toKey(target);
        for (const auto& w : dict->words_)
            if (toKey(w) == match)
                solutions->push_back(w);
        if (sort && !solutions->empty())
			SortRarest(solutions);
    }

/*IF--------------------------------------------------------------------------
public Regex_MatchWords
    Find words matching a pattern
&inputs
pattern is string
    The word or string to match
&optional
dict is handle Lexicon
    All the words
more_words is string[]
	Additional words to consider
sort is boolean (true)
	If true, output is sorted to display words with less common letters first
&outputs
solutions is string[]
    Words which can be transformed to the target via reversible letter substitution
-IF-------------------------------------------------------------------------*/

    void Regex_MatchWords
        (const String_& pattern,
         const Handle_<Lexicon_>& dict,
         const Vector_<String_>& more_words,
		 bool sort,
         Vector_<String_>* solutions)
    {
        std::regex re(pattern);
        if (dict)
        {
            for (const auto& w : dict->words_)
                if (std::regex_search(w, re))
                    solutions->push_back(w);
        }
        for (const auto& w : more_words)
            if (std::regex_search(w, re))
                solutions->push_back(w);

        if (sort && !solutions->empty())
            SortRarest(solutions);
    }

/*IF--------------------------------------------------------------------------
public Nearby_Letter
    Given letters, find a common neighboring letter
&inputs
letters is string
    The neighbors
dict is handle Lexicon
    All the words
rand_in is number
    &$ > 0.0 && $ < 1.0
    A random in (0, 1)
&optional
&outputs
neighbor is string
    The good-neighbor letter
-IF-------------------------------------------------------------------------*/

	void Nearby_Letter
        (const String_& letters,
         const Handle_<Lexicon_>& dict,
		 double rand_in,
         String_* neighbor)
    {
        std::array<int, 26> freq = {};
        for (const auto& w : dict->words_)
        {
			for (int ii = 0; ii < w.size(); ++ii)
			{
                if (letters.find(w[ii]) != String_::npos)
                    continue;
				if (ii > 0 && letters.find(w[ii - 1]) != String_::npos)
					++freq[w[ii] - 'a'];
				if (ii + 1 < w.size() && letters.find(w[ii + 1]) != String_::npos)
					++freq[w[ii] - 'a'];
			}
        }
        int which = AsInt(rand_in * Accumulate(freq));
        int loc = 0;
		while (loc < freq.size() && which >= freq[loc])
			which -= freq[loc++];
		*neighbor = String_(1, 'a' + static_cast<char>(loc));
    }
}

#include "MG_Lexicon_New_public.inc"
#include "MG_Synchrotron_Solutions_public.inc"
#include "MG_Synchrotron_MatchWords_public.inc"
#include "MG_Regex_MatchWords_public.inc"
#include "MG_Nearby_Letter_public.inc"
