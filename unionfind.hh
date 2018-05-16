#ifndef UNIONFIND_HH
#define UNIONFIND_HH

#include <unordered_map>

/**
 * UnionFind structure for connected component recognition.
 *
 * Connected component are sotred as trees and are reprented by one of the
 * elements of the component (root). By default all elements are root. Merging
 * is done by linking the trees (creates father/son a link between roots).
 *
 * When merging, we have to ensure we are looking at different trees to avoid
 * linking a root to itself and break the tree structure.
 *
 * When lokking for the root, we update all links from father to the root,
 * hence cutting on the tree depth for futur calls.
 */
template<typename T, class Container = std::unordered_map<T,T>>
class UnionFind
{
	public:
		UnionFind(const Container& cont         ) : m_mapping(cont)            {}
		UnionFind(Container&& cont = Container()) : m_mapping(std::move(cont)) {}
		UnionFind(const UnionFind&              ) = default;
		UnionFind(UnionFind&&                   ) = default;
		UnionFind& operator=(const UnionFind&   ) = default;
		UnionFind& operator=(UnionFind&&        ) = default;
		/**
		 * Find the root of the component in which `a` is
		 */
		const T& find(const T& a)
		{
			try
			{
				T& ra = m_mapping.at(a); // throws is no father
				ra = this->find(ra);     // find root and update father
				return ra;               // return father
			}
			catch (const std::out_of_range&)
			{
				return a;                // if no father, we are looking at the root
			}
		}
		/**
		 * Merge the components of `a` and `b`
		 */
		const UnionFind& merge(const T& a, const T& b)
		{
			const T& ra = this->find(a);  // ra is root of `a` → not in mapping
			const T& rb = this->find(b);  // rb is root of `b` → not in mapping
			if (ra != rb)                 // avoid creating loops
			{
				m_mapping.emplace(rb, ra); // add rb -> ra
			}
			return *this;
		}
	private:
		Container m_mapping;
};

#endif
