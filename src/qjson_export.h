/*  This file is part of the KDE project
    Copyright (C) 2009 Pino Toscano <pino@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License version 2.1, as published by the Free Software Foundation.
    

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef QJSON_EXPORT_H
#define QJSON_EXPORT_H

#include <QtCore/qglobal.h>

#ifndef QJSON_EXPORT
# if defined(QJSON_MAKEDLL)
   /* We are building this library */
#  define QJSON_EXPORT Q_DECL_EXPORT
# else
   /* We are using this library */
#  define QJSON_EXPORT Q_DECL_IMPORT
# endif
#endif

#endif
