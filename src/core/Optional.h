/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef OPTIONAL_H
#define OPTIONAL_H

/*
 * This utility class is for providing basic support for an option type.
 * It can be replaced by std::optional (C++17) or
 * std::experimental::optional (C++11) when they become fully supported
 * by all the compilers.
 */

template <typename T>
class Optional
{
public:
   
    // None
    Optional() :
        m_hasValue(false),
        m_value()
    { };   
 
    // Some T
    Optional(const T& value) :
        m_hasValue(true),
        m_value(value)
    { };

    // Copy
    Optional(const Optional& other) :
        m_hasValue(other.m_hasValue),
        m_value(other.m_value)
    { };

    const Optional& operator=(const Optional& other)
    {
        m_hasValue = other.m_hasValue;
        m_value = other.m_value;
        return *this;
    }

    bool operator==(const Optional& other) const
    {
        if(m_hasValue)
            return other.m_hasValue && m_value == other.m_value;
        else
            return !other.m_hasValue;
    }

    bool operator!=(const Optional& other) const
    {
        return !(*this == other);
    }

    bool hasValue() const
    {
        return m_hasValue;
    }

    T valueOr(const T& other) const
    {
        return m_hasValue ? m_value : other;
    }
    
    Optional static makeOptional(const T& value)
    {
        return Optional(value);
    }


private:

    bool m_hasValue;
    T m_value;
};

#endif // OPTIONAL_H
