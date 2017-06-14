// Base32 implementation
// Source: https://github.com/google/google-authenticator-libpam/blob/master/src/base32.h
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
// Modifications Copyright 2017 KeePassXC team <team@keepassxc.org>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BASE32_H
#define BASE32_H

#include <QtCore/qglobal.h>
#include <QByteArray>

class Base32
{
public:
    Base32();
    static QByteArray base32_decode(const QByteArray encoded);
};


#endif //BASE32_H
