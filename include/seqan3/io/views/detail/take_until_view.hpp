// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2021, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2021, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
// -----------------------------------------------------------------------------------------------------

/*!\file
 * \author Hannes Hauswedell <hannes.hauswedell AT fu-berlin.de>
 * \brief Provides seqan3::views::take_until and seqan3::views::take_until_or_throw.
 */

#pragma once

#include <seqan3/std/algorithm>
#include <seqan3/std/concepts>
#include <seqan3/std/iterator>
#include <seqan3/std/ranges>
#include <seqan3/std/type_traits>

#include <seqan3/core/detail/iterator_traits.hpp>
#include <seqan3/core/range/detail/adaptor_from_functor.hpp>
#include <seqan3/core/range/detail/inherited_iterator_base.hpp>
#include <seqan3/core/range/type_traits.hpp>
#include <seqan3/core/semiregular_box.hpp>
#include <seqan3/io/exception.hpp>
#include <seqan3/utility/range/concept.hpp>
#include <seqan3/utility/type_traits/detail/transformation_trait_or.hpp>

namespace seqan3::detail
{

// ============================================================================
//  view_take_until
// ============================================================================

/*!\brief The type returned by seqan3::detail::take_until and seqan3::detail::take_until_or_throw.
 * \tparam urng_t    The type of the underlying range, must model std::ranges::view.
 * \tparam fun_t     Type of the callable that will be evaluated on every member; must model
 *                   std::invocable with std::ranges::range_reference_t<urng_t> as argument and return `bool`.
 * \tparam or_throw  Whether to throw an exception when the input is exhausted before the end of line is reached.
 * \implements std::ranges::view
 * \implements std::ranges::random_access_range
 * \ingroup io_views
 *
 * \details
 *
 * Note that most members of this class are generated by ranges::view_interface which is not yet documented here.
 */
template <std::ranges::view urng_t, typename fun_t, bool or_throw, bool and_consume>
class view_take_until : public std::ranges::view_interface<view_take_until<urng_t, fun_t, or_throw, and_consume>>
{
private:

    static_assert(std::invocable<fun_t, std::ranges::range_reference_t<urng_t>>,
                  "The functor type for detail::take_until must model"
                  "std::invocable<fun_t, std::ranges::range_reference_t<urng_t>>.");
    static_assert(std::convertible_to<std::result_of_t<fun_t&&(std::ranges::range_reference_t<urng_t>)>, bool>,
                  "The result type of the functor for detail::take_until must be a boolean.");

    //!\brief The underlying range.
    urng_t urange;

    //!\brief The functor.
    ranges::semiregular_t<fun_t> fun;

    //!\brief Whether this view is const_iterable or not.
    static constexpr bool const_iterable = const_iterable_range<urng_t> &&
                                           std::regular_invocable<fun_t, std::ranges::range_reference_t<urng_t>>;

    template <bool const_range>
    using basic_iterator = seqan3::detail::maybe_const_iterator_t<const_range, urng_t>;

    template <bool const_range>
    class basic_sentinel;

    template <bool const_range>
    class basic_consume_iterator;

    template <bool const_range>
    using basic_consume_sentinel = seqan3::detail::maybe_const_sentinel_t<const_range, urng_t>;

public:
    /*!\name Constructors, destructor and assignment
     * \{
     */
    view_take_until()                                                  = default; //!< Defaulted.
    constexpr view_take_until(view_take_until const & rhs)             = default; //!< Defaulted.
    constexpr view_take_until(view_take_until && rhs)                  = default; //!< Defaulted.
    constexpr view_take_until & operator=(view_take_until const & rhs) = default; //!< Defaulted.
    constexpr view_take_until & operator=(view_take_until && rhs)      = default; //!< Defaulted.
    ~view_take_until()                                                 = default; //!< Defaulted.

    /*!\brief Construct from another range.
     * \param[in] _urange The underlying range.
     * \param[in] _fun    The functor that acts as termination criterium.
     */
    view_take_until(urng_t _urange, fun_t _fun)
        : urange{std::move(_urange)}, fun{std::forward<fun_t>(_fun)}
    {}

    /*!\brief Construct from another viewable_range.
     * \tparam rng_t      Type of the passed range; `urng_t` must be constructible from this.
     * \param[in] _urange The underlying range.
     * \param[in] _fun    The functor that acts as termination criterium.
     */
    template <std::ranges::viewable_range rng_t>
    //!\cond
        requires std::constructible_from<rng_t, std::views::all_t<rng_t>>
    //!\endcond
    view_take_until(rng_t && _urange, fun_t _fun)
        : view_take_until{std::views::all(std::forward<rng_t>(_urange)), std::move(_fun)}
    {}
    //!\}

    /*!\name Iterators
     * \{
     */
    /*!\brief Returns an iterator to the first element of the container.
     * \returns Iterator to the first element.
     *
     * If the container is empty, the returned iterator will be equal to seqan3::detail::take_until::end().
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    auto begin() noexcept
    {
        if constexpr (and_consume && !std::ranges::forward_range<urng_t>)
            return basic_consume_iterator<false>{std::ranges::begin(urange),
                                                 static_cast<fun_t &>(fun),
                                                 std::ranges::end(urange)};
        else
            return basic_iterator<false>{std::ranges::begin(urange)};
    }

    //!\copydoc begin()
    auto begin() const noexcept
        requires const_iterable
    {
        if constexpr (and_consume && !std::ranges::forward_range<urng_t const>)
            return basic_consume_iterator<true>{std::ranges::cbegin(urange),
                                                static_cast<fun_t const &>(fun),
                                                std::ranges::cend(urange)};
        else
            return basic_iterator<true>{std::ranges::cbegin(urange)};
    }

    /*!\brief Returns an iterator to the element following the last element of the range.
     * \returns Iterator to the end.
     *
     * This element acts as a placeholder; attempting to dereference it results in undefined behaviour.
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    auto end() noexcept
    {
        if constexpr (and_consume && !std::ranges::forward_range<urng_t>)
            return basic_consume_sentinel<false>{std::ranges::end(urange)};
        else
            return basic_sentinel<false>{std::ranges::end(urange), fun};
    }

    //!\copydoc end()
    auto end() const noexcept
        requires const_iterable
    {
        if constexpr (and_consume && !std::ranges::forward_range<urng_t const>)
            return basic_consume_sentinel<true>{std::ranges::cend(urange)};
        else
            return basic_sentinel<true>{std::ranges::cend(urange), static_cast<fun_t const &>(fun)};
    }
    //!\}
};

//!\brief Type deduction guide that strips references.
//!\relates seqan3::detail::view_take_until
template <typename urng_t, typename fun_t, bool or_throw = false, bool and_consume = false>
view_take_until(urng_t &&, fun_t) -> view_take_until<std::views::all_t<urng_t>, fun_t, or_throw, and_consume>;

//!\brief Special iterator type used when consuming behaviour is selected.
//!\tparam rng_t Should be `urng_t` for defining #iterator and `urng_t const` for defining #const_iterator.
template <std::ranges::view urng_t, typename fun_t, bool or_throw, bool and_consume>
template <bool const_range>
class view_take_until<urng_t, fun_t, or_throw, and_consume>::basic_consume_iterator :
    public inherited_iterator_base<basic_consume_iterator<const_range>,
                                   seqan3::detail::maybe_const_iterator_t<const_range, urng_t>>
{
private:
    //!\brief The iterator type of the underlying range.
    using underlying_iterator_t = seqan3::detail::maybe_const_iterator_t<const_range, urng_t>;
    //!\brief The CRTP wrapper type.
    using base_t = inherited_iterator_base<basic_consume_iterator, underlying_iterator_t>;

    //!\brief Auxiliary type.
    using predicate_ref_t = std::conditional_t<const_range,
                                               std::remove_reference_t<fun_t> const &,
                                               std::remove_reference_t<fun_t> &>;
    //!\brief Reference to the functor stored in the view.
    seqan3::semiregular_box_t<predicate_ref_t> fun;

    //!\brief The sentinel type is identical to that of the underlying range.
    using underlying_sentinel_t = seqan3::detail::maybe_const_sentinel_t<const_range, urng_t>;

    //!\brief Whether this iterator has reached the end (cache is only used on pure input ranges).
    underlying_sentinel_t underlying_sentinel;

    //!\brief Whether the end was reached by evaluating the functor.
    bool at_end_gracefully = false;

public:
    /*!\name Constructors, destructor and assignment
     * \brief Exceptions specification is implicitly inherited.
     * \{
     */
    constexpr basic_consume_iterator() = default; //!< Defaulted.
    constexpr basic_consume_iterator(basic_consume_iterator const & rhs) = default; //!< Defaulted.
    constexpr basic_consume_iterator(basic_consume_iterator && rhs) = default; //!< Defaulted.
    constexpr basic_consume_iterator & operator=(basic_consume_iterator const & rhs) = default; //!< Defaulted.
    constexpr basic_consume_iterator & operator=(basic_consume_iterator && rhs) = default; //!< Defaulted.
    ~basic_consume_iterator() = default; //!< Defaulted.

    //!\brief Constructor that delegates to the CRTP layer and initialises the callable.
    basic_consume_iterator(underlying_iterator_t it,
                           predicate_ref_t _fun,
                           underlying_sentinel_t sen) noexcept(noexcept(base_t{it})) :
        base_t{std::move(it)}, fun{_fun}, underlying_sentinel{std::move(sen)}
    {
        if ((this->base() != underlying_sentinel) && fun(**this))
        {
            at_end_gracefully = true;
            ++(*this);
        }
    }
    //!\}

    /*!\name Associated types
     * \brief All are derived from the underlying_iterator_t.
     * \{
     */
    using difference_type = std::iter_difference_t<underlying_iterator_t>; //!< From base.
    using value_type = std::iter_value_t<underlying_iterator_t>; //!< From base.
    using reference = std::iter_reference_t<underlying_iterator_t>; //!< From base.
    using pointer = detail::iter_pointer_t<underlying_iterator_t>; //!< From base.
    using iterator_category = std::input_iterator_tag; //!< Always input.
    //!\}

    /*!\name Arithmetic operators
     * \brief seqan3::detail::inherited_iterator_base operators are used unless specialised here.
     * \{
     */
    //!\brief Override pre-increment to implement consuming behaviour.
    basic_consume_iterator & operator++()
        noexcept(noexcept(++std::declval<base_t &>()) &&
                 noexcept(std::declval<underlying_iterator_t &>() != std::declval<underlying_sentinel_t &>()) &&
                 noexcept(fun(std::declval<reference>())))
    {
        base_t::operator++();

        while ((this->base() != underlying_sentinel) && fun(**this))
        {
            at_end_gracefully = true;
            base_t::operator++();
        }

        return *this;
    }

    //!\brief Post-increment implemented via pre-increment.
    basic_consume_iterator operator++(int)
        noexcept(noexcept(++std::declval<basic_consume_iterator &>()) &&
                 std::is_nothrow_copy_constructible_v<basic_consume_iterator>)
    {
        basic_consume_iterator cpy{*this};
        ++(*this);
        return cpy;
    }
    //!\}
    /*!\name Comparison operators
     * \brief We define comparison against self and against the sentinel.
     * \{
     */
    //!\brief Return the saved at_end state.
    bool operator==(basic_consume_sentinel<const_range> const &) const
        noexcept(!or_throw &&
                 noexcept(std::declval<underlying_iterator_t &>() != std::declval<underlying_sentinel_t &>()) &&
                 noexcept(fun(std::declval<reference>())))
    {
        if (at_end_gracefully)
            return true;

        if (this->base() == underlying_sentinel)
        {
            if constexpr (or_throw)
                throw unexpected_end_of_input{"Reached end of input before functor evaluated to true."};
            else
                return true;
        }

        return fun(**this);
    }

    //!\brief Return the saved at_end state.
    friend bool operator==(basic_consume_sentinel<const_range> const & lhs, basic_consume_iterator const & rhs)
        noexcept(noexcept(rhs == lhs))
    {
        return rhs == lhs;
    }

    //!\brief Return the saved at_end state.
    bool operator!=(basic_consume_sentinel<const_range> const & rhs) const
        noexcept(noexcept(std::declval<basic_consume_iterator &>() == rhs))
    {
        return !(*this == rhs);
    }

    //!\brief Return the saved at_end state.
    friend bool operator!=(basic_consume_sentinel<const_range> const & lhs, basic_consume_iterator const & rhs)
        noexcept(noexcept(rhs != lhs))
    {
        return rhs != lhs;
    }
    //!\}
};

//!\brief The sentinel type of take_until, provides the comparison operators.
template <std::ranges::view urng_t, typename fun_t, bool or_throw, bool and_consume>
template <bool const_range>
class view_take_until<urng_t, fun_t, or_throw, and_consume>::basic_sentinel
{
private:
    //!\brief The sentinel type of the underlying range.
    using underlying_sentinel_t = seqan3::detail::maybe_const_sentinel_t<const_range, urng_t>;
    //!\brief Auxiliary type.
    using predicate_ref_t = std::conditional_t<const_range,
                                               std::remove_reference_t<fun_t> const &,
                                               std::remove_reference_t<fun_t> &>;

    //!\brief The actual end of the underlying range.
    underlying_sentinel_t underlying_sentinel{};

    //!\brief Reference to the predicate stored in the view.
    seqan3::semiregular_box_t<predicate_ref_t> predicate{};

public:
    /*!\name Constructors, destructor and assignment
     * \{
     */
    basic_sentinel() = default; //!< Defaulted.
    basic_sentinel(basic_sentinel const &) = default; //!< Defaulted.
    basic_sentinel(basic_sentinel &&) = default; //!< Defaulted.
    basic_sentinel & operator=(basic_sentinel const &) = default; //!< Defaulted.
    basic_sentinel & operator=(basic_sentinel &&) = default; //!< Defaulted.
    ~basic_sentinel() = default; //!< Defaulted.

    /*!\brief Construct from a sentinel and a predicate.
     * \param[in] underlying_sentinel  The actual end of the underlying range.
     * \param[in] predicate      Reference to the predicate stored in the view.
     */
    explicit basic_sentinel(underlying_sentinel_t underlying_sentinel, predicate_ref_t predicate) :
        underlying_sentinel{std::move(underlying_sentinel)},
        predicate{predicate}
    {}

    //!\brief Construct from a not const range a const range.
    basic_sentinel(basic_sentinel<!const_range> other)
        requires const_range && std::convertible_to<std::ranges::sentinel_t<urng_t>, underlying_sentinel_t>
        : underlying_sentinel{std::move(other.underlying_sentinel)},
          predicate{other.predicate}
    {}
    //!\}

    /*!\name Comparison operators
     * \{
     */

    //!\brief Compares `lhs` with `rhs` for equality.
    friend bool operator==(basic_iterator<const_range> const & lhs, basic_sentinel const & rhs)
    {
        // Actual comparison delegated to lhs base
        if (lhs == rhs.underlying_sentinel)
        {
            if constexpr (or_throw)
                throw unexpected_end_of_input{"Reached end of input before functor evaluated to true."};
            else
                return true;
        }

        return rhs.predicate(*lhs);
    }

    //!\brief Compares `lhs` with `rhs` for equality.
    friend bool operator==(basic_sentinel const & lhs, basic_iterator<const_range> const & rhs)
    {
        return rhs == lhs;
    }

    //!\brief Compares `lhs` with `rhs` for inequality.
    friend bool operator!=(basic_iterator<const_range> const & lhs, basic_sentinel const & rhs)
    {
        return !(lhs == rhs);
    }

    //!\brief Compares `lhs` with `rhs` for inequality.
    friend bool operator!=(basic_sentinel const & lhs, basic_iterator<const_range> const & rhs)
    {
        return rhs != lhs;
    }

    //!\brief Compares `lhs` with `rhs` for equality.
    template <bool other_const_range = !const_range>
        requires (std::sentinel_for<underlying_sentinel_t, basic_iterator<other_const_range>>)
    friend bool operator==(basic_iterator<other_const_range> const & lhs, basic_sentinel const & rhs)
    {
        // Actual comparison delegated to lhs base
        if (lhs == rhs.underlying_sentinel)
        {
            if constexpr (or_throw)
                throw unexpected_end_of_input{"Reached end of input before functor evaluated to true."};
            else
                return true;
        }

        return rhs.predicate(*lhs);
    }

    //!\brief Compares `lhs` with `rhs` for equality.
    template <bool other_const_range = !const_range>
        requires (std::sentinel_for<underlying_sentinel_t, basic_iterator<other_const_range>>)
    friend bool operator==(basic_sentinel const & lhs, basic_iterator<other_const_range> const & rhs)
    {
        return rhs == lhs;
    }

    //!\brief Compares `lhs` with `rhs` for inequality.
    template <bool other_const_range = !const_range>
        requires (std::sentinel_for<underlying_sentinel_t, basic_iterator<other_const_range>>)
    friend bool operator!=(basic_iterator<other_const_range> const & lhs, basic_sentinel const & rhs)
    {
        return !(lhs == rhs);
    }

    //!\brief Compares `lhs` with `rhs` for inequality.
    template <bool other_const_range = !const_range>
        requires (std::sentinel_for<underlying_sentinel_t, basic_iterator<other_const_range>>)
    friend bool operator!=(basic_sentinel const & lhs, basic_iterator<other_const_range> const & rhs)
    {
        return rhs != lhs;
    }
    //!\}
};

// ============================================================================
//  take_until_fn (adaptor definition)
// ============================================================================

/*!\brief View adaptor definition for detail::take_until and detail::take_until_or_throw.
 * \tparam or_throw Whether to throw an exception when the input is exhausted before the end of line is reached.
 */
template <bool or_throw, bool and_consume>
struct take_until_fn
{
    //!\brief Store the arguments and return a range adaptor closure object.
    template <typename fun_t>
    constexpr auto operator()(fun_t && fun) const
    {
        return adaptor_from_functor{*this, std::forward<fun_t>(fun)};
    }

    /*!\brief Call the view's constructor with the given parameters.
     * \tparam    urng_t The underlying range type; must model std::ranges::view.
     * \tparam    fun_t  The type of the callable; concept checks done in class.
     * \param[in] urange The underlying range.
     * \param[in] fun    The callable that will be evaluated on every element.
     * \returns An instance of seqan3::detail::view_take_until.
     */
    template <std::ranges::viewable_range urng_t, typename fun_t>
    constexpr auto operator()(urng_t && urange, fun_t && fun) const
    {
        return view_take_until<std::views::all_t<urng_t>, fun_t, or_throw, and_consume>
        {
            std::views::all(std::forward<urng_t>(urange)),
            std::forward<fun_t>(fun)
        };
    }
};

} // namespace seqan3::detail

// ============================================================================
//  detail::take_until (adaptor instance definition)
// ============================================================================

namespace seqan3::detail
{
/*!\brief               A view adaptor that returns elements from the underlying range until the functor evaluates to
 *                      true (or the end of the underlying range is reached).
 * \tparam urng_t       The type of the range being processed. See below for requirements. [template parameter is
 *                      omitted in pipe notation]
 * \tparam fun_t        The type of the functor; must model std::invocable with std::ranges::range_reference_t<urng_t>
 *                      and return a type convertible to `bool`.
 * \param[in] urange    The range being processed. [parameter is omitted in pipe notation]
 * \param[in] fun       The functor.
 * \returns             All elements of the underlying range up until (but excluding) the element that evaluates the
 *                      functor to true.
 * \ingroup io_views
 *
 * \details
 *
 * \header_file{seqan3/io/views/detail/take_until_view.hpp}
 *
 * ### View properties
 *
 * | Concepts and traits              | `urng_t` (underlying range type)      | `rrng_t` (returned range type)                     |
 * |----------------------------------|:-------------------------------------:|:--------------------------------------------------:|
 * | std::ranges::input_range         | *required*                            | *preserved*                                        |
 * | std::ranges::forward_range       |                                       | <i>preserved</i>¹                                  |
 * | std::ranges::bidirectional_range |                                       | <i>preserved</i>¹                                  |
 * | std::ranges::random_access_range |                                       | <i>preserved</i>¹                                  |
 * | std::ranges::contiguous_range    |                                       | <i>preserved</i>¹                                  |
 * |                                  |                                       |                                                    |
 * | std::ranges::viewable_range      | *required*                            | *guaranteed*                                       |
 * | std::ranges::view                |                                       | *guaranteed*                                       |
 * | std::ranges::sized_range         |                                       | *lost*                                             |
 * | std::ranges::common_range        |                                       | *lost*                                             |
 * | std::ranges::output_range        |                                       | *preserved*                                        |
 * | seqan3::const_iterable_range     |                                       | <i>preserved</i>¹                                  |
 * |                                  |                                       |                                                    |
 * | std::ranges::range_reference_t   |                                       | std::ranges::range_reference_t<urng_t>             |
 *
 * See the \link views views submodule documentation \endlink for detailed descriptions of the view properties.
 *
 * ¹ The marked properties are only *preserved* if the specified functor models
 * `std::regular_invocable<fun_t, std::ranges::range_reference_t<urng_t>`, i.e. applying the functor doesn't change the functor.
 * If the functor only models `std::invocable` and not `std::regular_invocable` these concepts are *lost*.
 *
 * Throwing: `seqan3::detail::take_until_or_throw` and `seqan3::detail::take_until_or_throw_and_consume` throw an exception
 * if the end of the underlying range is reached before their own termination criterium is met. This is useful
 * if you want a "strict" evaluation of the functor.
 *
 * Consuming: `seqan3::detail::take_until_and_consume` and `seqan3::detail::take_until_or_throw_and_consume` behave the
 * same as their non-consuming counter-parts if the underlying range models at least `std::ranges::forward_range`.
 * If, however, the underlying range is a pure `std::ranges::input_range`, the view will keep moving the underlying
 * iterator forward as long as the termination criterium holds and the underlying range is not at end.
 * This is useful for string tokenisation among other things.
 *
 * ### Example
 *
 * \include test/snippet/io/views/detail/take_until_view.cpp
 *
 * \hideinitializer
 */
inline auto constexpr take_until = take_until_fn<false, false>{};

// ============================================================================
//  detail::take_until_or_throw (adaptor instance definition)
// ============================================================================

/*!\brief A view adaptor that returns elements from the underlying range until the functor evaluates to true
 *        (**throws** if the end of the underlying range is reached).
 * \throws seqan3::unexpected_end_of_input If the underlying range contains no element that satisfies the functor.
 * \ingroup io_views
 *
 * \copydetails seqan3::detail::take_until
 * \hideinitializer
 */
inline auto constexpr take_until_or_throw = take_until_fn<true, false>{};

// ============================================================================
//  detail::take_until_and_consume (adaptor instance definition)
// ============================================================================

/*!\brief A view adaptor that returns elements from the underlying range until the functor evaluates to true
 *        (or the end of the underlying range is reached and consumes the end in single-pass ranges).
 * \throws seqan3::unexpected_end_of_input If the underlying range contains no element that satisfies the functor.
 * \ingroup io_views
 *
 * \copydetails seqan3::detail::take_until
 * \hideinitializer
 */
inline auto constexpr take_until_and_consume = take_until_fn<false, true>{};

// ============================================================================
//  detail::take_until_or_throw_and_consume (adaptor instance definition)
// ============================================================================

/*!\brief A view adaptor that returns elements from the underlying range until the functor evaluates to true
 *        (**throws** if the end of the underlying range is reached and consumes the end in single-pass ranges).
 * \throws seqan3::unexpected_end_of_input If the underlying range contains no element that satisfies the functor.
 * \ingroup io_views
 *
 * \copydetails seqan3::detail::take_until_and_consume
 * \hideinitializer
 */
inline auto constexpr take_until_or_throw_and_consume = take_until_fn<true, true>{};

} // namespace seqan3::detail
