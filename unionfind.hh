#ifndef UNIONFIND_HH
#define UNIONFIND_HH

#include <map>
#include <unordered_map>

/**
 * Template checking for operators == and !=
 */
/* fallback for std::negation which is c++17 */
// template<class B> struct negation : std::integral_constant<bool, !bool(B::value)> {};
/* fallback type for undeclared operators, will be checked below */
template<typename T, typename Arg> std::false_type operator== (const T&, const Arg&);
template<typename T, typename Arg> std::true_type  operator!= (const T&, const Arg&);
/* check the decltype of the comparaison operator against fallbacks */
template<typename T> struct hasEqOp  : std::negation<std::is_same<decltype(*(T*)(0) == *(T*)(0)), std::false_type>> {};
template<typename T> struct hasNeqOp : std::negation<std::is_same<decltype(*(T*)(0) != *(T*)(0)), std::true_type >> {};
/* specialization for void (above pattern is invalid : void* cannot de dereferenced to a void object) */
template<> struct hasEqOp<void>  : std::false_type {};
template<> struct hasNeqOp<void> : std::false_type {};

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
template<typename T>
class UnionFind
{
		static_assert(std::is_copy_constructible<T>::value); // required by container::insert
		static_assert(hasNeqOp<T>::value);                   // required by merge

	public:
		// use unordered_map if T is hashable, map otherwise
		using container = typename std::conditional<
			std::is_constructible<std::hash<T>>::value,
			std::unordered_map<T,T>,
			std::map<T,T>
		>::type;

	public:
		UnionFind()                 = default;
		UnionFind(const UnionFind&) = default;
		UnionFind(UnionFind&&)      = default;

		UnionFind& operator=(const UnionFind&) = default;
		UnionFind& operator=(UnionFind&&)      = default;

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
		container m_mapping;
};

#endif
