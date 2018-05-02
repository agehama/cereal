/*! \file boost_variant.hpp
\brief Support for boost::variant
\ingroup OtherTypes */
/*
Copyright (c) 2014, Randolph Voorhies, Shane Grant
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of cereal nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES OR SHANE GRANT BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CEREAL_TYPES_BOOST_VARIANT_HPP_
#define CEREAL_TYPES_BOOST_VARIANT_HPP_

#include "cereal/cereal.hpp"

#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/empty.hpp>

#include <boost/variant.hpp>

namespace cereal
{
	namespace variant_detail
	{
		//! @internal
		template <class Archive>
		struct variant_save_visitor : boost::static_visitor<>
		{
			variant_save_visitor(Archive & ar_) : ar(ar_) {}

			template<class T>
			void operator()(T const & value) const
			{
				ar(CEREAL_NVP_("data", value));
			}

			Archive & ar;
		};

		//! @internal
		template<class S>
		struct variant_impl {

			struct load_null {
				template<class Archive, class V>
				static void invoke(
					Archive & /*ar*/,
					int /*which*/,
					V & /*v*/
				) {}
			};

			struct load_impl {
				template<class Archive, class V>
				static void invoke(
					Archive & ar,
					int which,
					V & v
				) {
					if (which == 0) {
						// note: A non-intrusive implementation (such as this one)
						// necessary has to copy the value.  This wouldn't be necessary
						// with an implementation that de-serialized to the address of the
						// aligned storage included in the variant.
						typedef typename boost::mpl::front<S>::type head_type;
						head_type value;
						ar(CEREAL_NVP_("data", value));
						v = value;
						return;
					}
					typedef typename boost::mpl::pop_front<S>::type type;
					variant_impl<type>::load(ar, which - 1, v);
				}
			};

			template<class Archive, class V>
			static void load(
				Archive & ar,
				int which,
				V & v
			) {
				typedef typename boost::mpl::eval_if<boost::mpl::empty<S>,
					boost::mpl::identity<load_null>,
					boost::mpl::identity<load_impl>
				>::type typex;
				typex::invoke(ar, which, v);
			}
		};
	} // namespace variant_detail

	  //! Saving for boost::variant
	template <class Archive, BOOST_VARIANT_ENUM_PARAMS(/* typename */ class T)> inline
		void CEREAL_SAVE_FUNCTION_NAME(Archive & ar, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const & variant)
	{
		int32_t which = variant.which();
		ar(CEREAL_NVP_("which", which));
		variant_detail::variant_save_visitor<Archive> visitor(ar);
		variant.apply_visitor(visitor);
	}

	//! Loading for boost::variant
	template <class Archive, BOOST_VARIANT_ENUM_PARAMS(/* typename */ class T)> inline
		void CEREAL_LOAD_FUNCTION_NAME(Archive & ar, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>& variant)
	{
		typedef typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types types;

		int32_t which;
		ar(CEREAL_NVP_("which", which));
		if (which >= boost::mpl::size<types>::value)
			throw Exception("Invalid 'which' selector when deserializing boost::variant");

		variant_detail::variant_impl<types>::load(ar, which, variant);
	}
} // namespace cereal

#endif // CEREAL_TYPES_BOOST_VARIANT_HPP_
