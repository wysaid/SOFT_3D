/*
@Author: wysaid
@Blog: blog.wysaid.org
@Date: 2013-10-31
*/
#ifndef _HT_STATICASSERT_H_
#define _HT_STATICASSERT_H_

#ifdef _USE_CPP11_STATIC_ASSERT_

#define htStaticAssert(value) static_assert(value, "Invalid Parameters!")

#else

#if defined(DEBUG) || defined(_DEBUG)

template<bool K>
struct HTStaticAssert_ ;

template<>
struct HTStaticAssert_<true> { int dummy; };

template<int n>
struct HTStaticAssert {};

#define htStaticAssert(value) do \
{\
	typedef HTStaticAssert<\
	sizeof(HTStaticAssert_<(bool)(value)>)\
	> HTStaticAssert__;\
} while (0)

#else

#define htStaticAssert(...) 

#endif

#endif

#endif //_HT_STATICASSERT_H_