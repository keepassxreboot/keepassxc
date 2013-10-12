/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *  Copyright (C) 2012 Intel Corporation
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSX_GLOBAL_H
#define KEEPASSX_GLOBAL_H

// mostly copied from qcompilerdetection.h which is part of Qt 5

#include <QtGlobal>

#ifdef Q_CC_CLANG
#  if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#    if __has_feature(cxx_strong_enums)
#      define COMPILER_CLASS_ENUM
#    endif
#    if __has_feature(cxx_constexpr)
#      define COMPILER_CONSTEXPR
#    endif
#    if __has_feature(cxx_decltype) /* && __has_feature(cxx_decltype_incomplete_return_types) */
#      define COMPILER_DECLTYPE
#    endif
#    if __has_feature(cxx_override_control)
#      define COMPILER_EXPLICIT_OVERRIDES
#    endif
#    if __has_feature(cxx_nullptr)
#      define COMPILER_NULLPTR
#    endif
#    if __has_feature(cxx_static_assert)
#      define COMPILER_STATIC_ASSERT
#    endif
#  endif
#endif // Q_CC_CLANG

#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && !defined(Q_CC_CLANG)
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403
#      define COMPILER_DECLTYPE
#      define COMPILER_STATIC_ASSERT
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 404
#      define COMPILER_CLASS_ENUM
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406
#      define COMPILER_CONSTEXPR
#      define COMPILER_NULLPTR
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
#      define COMPILER_EXPLICIT_OVERRIDES
#    endif
#  endif
#endif

/*
 * C++11 keywords and expressions
 */
#if !defined(Q_NULLPTR)
#  ifdef COMPILER_NULLPTR
#    define Q_NULLPTR nullptr
#  else
#    define Q_NULLPTR 0
#  endif
#endif

#if !defined(Q_DECL_CONSTEXPR)
#  ifdef COMPILER_CONSTEXPR
#    define Q_DECL_CONSTEXPR constexpr
#  else
#    define Q_DECL_CONSTEXPR
#  endif
#endif

#if !defined(Q_DECL_OVERRIDE) && !defined(Q_DECL_FINAL) && !defined(Q_DECL_FINAL_CLASS)
#  ifdef COMPILER_EXPLICIT_OVERRIDES
#    define Q_DECL_OVERRIDE override
#    define Q_DECL_FINAL final
#    ifdef  COMPILER_DECLTYPE
#      define Q_DECL_FINAL_CLASS final
#    else
#      define Q_DECL_FINAL_CLASS
#    endif
#  else
#    define Q_DECL_OVERRIDE
#    define Q_DECL_FINAL
#    define Q_DECL_FINAL_CLASS
#  endif
#endif

#if !defined(Q_STATIC_ASSERT) && !defined(Q_STATIC_ASSERT_X)
#ifdef COMPILER_STATIC_ASSERT
#define Q_STATIC_ASSERT(Condition) static_assert(static_cast<bool>(Condition), #Condition)
#define Q_STATIC_ASSERT_X(Condition, Message) static_assert(static_cast<bool>(Condition), Message)
#else
// Intentionally undefined
template <bool Test> class QStaticAssertFailure;
template <> class QStaticAssertFailure<true> {};

#define Q_STATIC_ASSERT_PRIVATE_JOIN(A, B) Q_STATIC_ASSERT_PRIVATE_JOIN_IMPL(A, B)
#define Q_STATIC_ASSERT_PRIVATE_JOIN_IMPL(A, B) A ## B
#define Q_STATIC_ASSERT(Condition) \
    enum {Q_STATIC_ASSERT_PRIVATE_JOIN(q_static_assert_result, __LINE__) = sizeof(QStaticAssertFailure<!!(Condition)>)}
#define Q_STATIC_ASSERT_X(Condition, Message) Q_STATIC_ASSERT(Condition)
#endif // COMPILER_STATIC_ASSERT
#endif // !defined(Q_STATIC_ASSERT) && !defined(Q_STATIC_ASSERT_X)

#if defined(Q_OS_WIN)
#  if defined(KEEPASSX_BUILDING_CORE)
#    define KEEPASSX_EXPORT Q_DECL_EXPORT
#  else
#    define KEEPASSX_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define KEEPASSX_EXPORT Q_DECL_EXPORT
#endif

#ifndef QUINT32_MAX
#define QUINT32_MAX 4294967295U
#endif

#endif // KEEPASSX_GLOBAL_H
