#ifndef SRC_PSST_H_
#define SRC_PSST_H_

#include <type_traits>
#include <ostream>

#include <utility>

namespace Psst{


template<typename B, template<typename...>class T>
struct bind2{
	template<typename A, typename ...C>
	using apply=T<A,B,C...>;
};
template<typename A, template<typename...>class T>
struct bind1{
	template<typename ...B>
	using apply=T<A,B...>;
};

template <typename U, template <typename ...> class ...BS>
struct ops:BS<U>...{};


// not really needed with structured bindings.
template <typename V, typename TAG>
struct strong { // can not merge ops here, because of initializers required for bases depending on incomplete type
	static_assert(std::is_object_v<V>,"must keep real values");
	using value_type=V;
	V val;

};


//template <typename U>
//struct is_strong{
//	template<typename T>
//	static char test(decltype(T::val)*);
//	template<typename T>
//	static long long test(...);
//	static inline constexpr bool value=sizeof(test<U>(nullptr))==sizeof(char) ; // sizeof(U) == sizeof the value
//};
//template<typename U>
//constexpr inline  bool is_strong_v=is_strong<U>::value;

namespace detail{
template <typename EBase>
struct is_ebo_impl{
	struct non_empty{ char x;};
	struct test:std::conditional_t<std::is_class_v<EBase> && !std::is_final_v<EBase>,EBase,non_empty> {
		char c;
	};
	static_assert(sizeof(non_empty)==sizeof(char),"structs have optimal size");
	static inline constexpr bool value=sizeof(test)==sizeof(non_empty);
};
}
template <typename EBase>
struct is_ebo: std::bool_constant<detail::is_ebo_impl<EBase>::value>{};

template <typename U>
constexpr inline bool is_ebo_v=is_ebo<U>::value;

template<typename U>
constexpr inline bool is_strong_v { std::is_class_v<U> && std::is_aggregate_v<U>}; // more universal something in there and not a simple one

namespace detail{
template <typename T,  std::enable_if_t<is_strong_v<T>,int> = 0>
constexpr auto membertype(T x) { // not meant to be evaluated, assumes is_class_v<T>
	auto [y]=x;
	return y;
}
template <typename T, std::enable_if_t<std::is_arithmetic_v<T>||std::is_enum_v<T>,int> = 0> // do not match non-aggregates or pointers
constexpr auto membertype(T x) { // not meant to be evaluated, assumes is_class_v<T>
	return x;
}
}
template <typename T>
using underlying_value_type = decltype(detail::membertype(std::declval<T>()));
template <typename T>
struct default_zero{
	constexpr auto operator()() const{
		return underlying_value_type<T>{};
	}
};

// should we declare absolute to be a relative unit with a given/arbitrary base or vice versa?
// chrono defines time_point in terms of duration and a clock which gives the starting point
// what is Size? absolute, then what is its relative companion.
// according to boost units that is what affine spaces are about.
// how would an absolute adapter look like? (experimental)

template <typename T, typename BASE=default_zero<T>>
struct absolute {
	constexpr inline static auto origin=BASE{}();
	T rel;
};


template <typename U>
struct is_absolute{
		template<typename T>
		static char test(decltype(T::origin)*);
		template<typename T>
		static long long test(...);
		static inline constexpr bool value=sizeof(test<U>(nullptr))==sizeof(char);
};
template<typename U>
constexpr inline  bool is_absolute_v=is_absolute<U>::value;


static_assert(!is_strong_v<int>,"int is no unit");
static_assert(!is_absolute_v<int>,"int is no absolute unit");


// ops templates

template <typename U>
struct Inc{
	friend constexpr auto operator++(U &rv) noexcept {
		auto &[val]=rv;
		++val;
		return rv;
	}
	friend constexpr auto operator++(U &rv,int) noexcept {
		auto res=rv;
		++rv;
		return res;
	}
};


template <typename U>
struct Eq{
	friend constexpr bool
	operator==(U const &l, U const& r) noexcept {
		auto const &[vl]=l;
		auto const &[vr]=r;
		return vl == vr;
	}
	friend constexpr bool
	operator!=(U const &l, U const& r) noexcept {
		return !(l==r);
	}
};
template <typename U, typename V, typename...>
struct EqWithImpl{
	friend constexpr bool
	operator==(U const &l, V const& r) noexcept {
		static_assert(is_strong_v<U>,"can only be applied to strong types with 1 member");
		static_assert(!std::is_same_v<U,V>,"can not be applied to identical types");
		auto const &[vl]=l;
		if constexpr(std::is_arithmetic_v<V>){
			return vl == r;
		}
		else{
			auto const &[vr]=r;
			return vl == vr;
		}
	}
	friend constexpr bool
	operator==(V const &l, U const& r) noexcept {
		return r == l;
	}
	friend constexpr bool
	operator!=(U const &l, V const& r) noexcept {
		return !(l==r);
	}
	friend constexpr bool
	operator!=(V const &l, U const& r) noexcept {
		return !(r==l);
	}
};

template <typename W>
using EqWith=bind2<W,EqWithImpl>;

template <typename U>
struct Cmp{
	friend constexpr bool
	operator<(U const &l, U const& r) noexcept {
		static_assert(is_strong_v<U>,"can only be applied to strong types with 1 member");
		auto const &[vl]=l;
		auto const &[vr]=r;
		return vl < vr;
	}
	friend constexpr bool
	operator>(U const &l, U const& r) noexcept {
		return r < l;
	}
	friend constexpr bool
	operator<=(U const &l, U const& r) noexcept {
		return !(r < l);
	}
	friend constexpr bool
	operator>=(U const &l, U const& r) noexcept {
		return !(l < r);
	}
};
template <typename U, typename V, typename ...>
struct CmpWithImpl{
	friend constexpr bool
	operator<(U const &l, V const& r) noexcept {
		static_assert(is_strong_v<U>,"can only be applied to strong types with 1 member");
		static_assert(!std::is_same_v<U,V>,"can not be applied to identical types");
		auto const &[vl]=l;

		if constexpr(is_strong_v<V>){
			auto const &[vr]=r;
			return vl < vr;
		}
		else
			return vl < r;
	}
	friend constexpr bool
	operator<(V const &l, U const& r) noexcept {
		static_assert(is_strong_v<U>,"can only be applied to strong types with 1 member");
		static_assert(!std::is_same_v<U,V>,"can not be applied to identical types");
		auto const &[vr]=r;
		if constexpr(is_strong_v<V>){
				auto const &[vl]=l;
			return vl < vr;
		}else
			return l < vr;
	}
	friend constexpr bool
	operator>(U const &l, V const& r) noexcept {
		return r > l;
	}
	friend constexpr bool
	operator>(V const &l, U const& r) noexcept {
		return r > l;
	}
	friend constexpr bool
	operator<=(U const &l, V const& r) noexcept {
		return !(r > l);
	}
	friend constexpr bool
	operator<=(V const &l, U const& r) noexcept {
		return !(r > l);
	}
	friend constexpr bool
	operator>=(U const &l, V const& r) noexcept {
		return !(l < r);
	}
	friend constexpr bool
	operator>=(V const &l, U const& r) noexcept {
		return !(l < r);
	}
};
template <typename W>
using CmpWith=bind2<W,CmpWithImpl>;


template <typename U>
struct Add {
	friend constexpr U&
	operator+=(U& l, U const &r) noexcept {
		auto &[vl]=l;
		auto const &[vr] = r;
		vl += vr;
		return l;
	}
	friend constexpr U
	operator+(U l, U const &r) noexcept {
		return l+=r;
	}
};
template <typename U>
struct Sub {
	friend constexpr U&
	operator-=(U& l, U const &r) noexcept {
		auto &[vl]=l;
		auto const &[vr] = r;
		vl -= vr;
		return l;
	}
	friend constexpr U
	operator-(U l, U const &r) noexcept {
		return l-=r;
	}
};
//todo other useful arithmetic - * with / by
// combination: arithmetic, relative, absolute_relative combination,
// absolute mix in tag? duration+absolutetag = time_point

template <typename U>
struct Out {
	friend std::ostream&
	operator<<(std::ostream &l, U const &r) {
		auto const &[v]=r;
		return l << v;
	}
};

namespace ___testing___{
struct bla:strong<int,bla>,ops<bla,Eq>{};
static_assert(is_strong_v<bla>,"bla is a unit");
static_assert(!is_absolute_v<bla>,"bla is absolute?");
using blu=absolute<bla>;
static_assert(is_absolute_v<blu>,"blu should be absolute");
static_assert(blu::origin==0,"blu origin is zero");
static_assert(blu{42}.rel==bla{42}, "rel accessible");
static_assert(std::is_same_v<int,underlying_value_type<bla>>,"..");

struct dummy{int i;};
static_assert(is_ebo_v<Add<dummy>>,"Add should be EBO enabled");
static_assert(is_ebo_v<Sub<dummy>>,"Mul should be EBO enabled");
static_assert(is_ebo_v<Out<dummy>>,"Out should be EBO enabled");
static_assert(is_ebo_v<Cmp<dummy>>,"Cmp should be EBO enabled");
static_assert(is_ebo_v<Eq<dummy>>,"Eq should be EBO enabled");
static_assert(is_ebo_v<ops<dummy,EqWith<int>::apply>>,"EqWith::apply should be EBO enabled");
static_assert(is_ebo_v<ops<dummy,CmpWith<int>::apply>>,"CmpWith::apply should be EBO enabled");

struct dummy_d:ops<dummy,Sub,Add> {
	double v;
};
static_assert(sizeof(double)==sizeof(dummy_d),"dummy_d should be same size as double");
}

}


#endif /* SRC_PSST_H_ */
